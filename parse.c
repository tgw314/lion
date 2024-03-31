#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    Object *var;
};

typedef struct Scope Scope;
struct Scope {
    Scope *next;
    VarScope *vars;
};

static Object *locals;
static Object *globals = &(Object){};

static Scope *scope = &(Scope){};

static Type *declspec();
static Type *declarator(Type *type, Token **ident_tok);

static void declaration_global();
static void function(Type *type, Token *tok);
static void params(Object *func);
static Node *stmt();
static Node *compound_stmt();
static Node *expr_stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *postfix();
static Node *primary();

static void enter_scope() {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->next = scope;
    scope = sc;
}

static void leave_scope() { scope = scope->next; }

static VarScope *push_scope(Object *var) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->var = var;
    sc->next = scope->vars;
    scope->vars = sc;
    return sc;
}

static Object *new_func(Type *type, Token *tok) {
    Object *func = calloc(1, sizeof(Object));
    func->type = type;
    func->name = strndup(tok->loc, tok->len);
    func->is_func = true;
    func->is_local = false;
    return func;
}

static Object *new_var(Type *type, Token *tok) {
    Object *var = calloc(1, sizeof(Object));
    var->type = type;
    var->name = strndup(tok->loc, tok->len);
    var->is_func = false;
    return var;
}

static Object *new_gvar(Type *type, Token *tok) {
    Object *gvar = new_var(type, tok);
    gvar->is_local = false;
    return gvar;
}

static Object *new_anon_gvar(Type *type) {
    static int index = 0;

    char *name = calloc(1, 20);
    sprintf(name, ".LC%d", index++);

    return new_gvar(type, &(Token){.loc = name, .len = strlen(name)});
}

static Object *new_string_literal(char *str) {
    Type *type = new_type_array(new_type(TY_CHAR), strlen(str) + 1);
    Object *gvar = new_anon_gvar(type);
    gvar->init_data = str;
    return gvar;
}

static Object *new_lvar(Type *type, Token *tok) {
    Object *lvar = new_var(type, tok);
    lvar->is_local = true;
    return lvar;
}

static Object *find_var(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->next) {
        for (VarScope *sc2 = sc->vars; sc2; sc2 = sc2->next) {
            if (sc2->var->name != NULL &&
                !strncmp(tok->loc, sc2->var->name, tok->len)) {
                return sc2->var;
            }
        }
    }
    return NULL;
}

static Object *find_var_scope(Token *tok) {
    for (VarScope *sc = scope->vars; sc; sc = sc->next) {
        if (sc->var->name != NULL &&
            !strncmp(tok->loc, sc->var->name, tok->len)) {
            return sc->var;
        }
    }
    return NULL;
}

static Object *find_func(Token *tok) {
    for (Object *func = globals; func; func = func->next) {
        if (func->is_func && func->name != NULL &&
            !strncmp(tok->loc, func->name, tok->len)) {
            return func;
        }
    }
    return NULL;
}

// ローカル変数を locals の先頭に追加する
static void add_lvar(Object *lvar) {
    lvar->next = locals;
    locals = lvar;
    push_scope(lvar);
}

// 関数とグローバル変数を globals の末尾に追加する
static void add_global(Object *global) {
    static Object *cur = NULL;

    for (cur = globals; cur; cur = cur->next) {
        if (cur->next == NULL) {
            cur = cur->next = global;
            push_scope(global);
            return;
        }
    }
}

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_node_num(Token *tok, int val) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_node_expr(NodeKind kind, Token *tok, Node *lhs, Node *rhs) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_add(Token *tok, Node *lhs, Node *rhs) {
    set_node_type(lhs);
    set_node_type(rhs);

    if (is_number(lhs->type) && is_number(rhs->type)) {
        return new_node_expr(ND_ADD, tok, lhs, rhs);
    }

    // 左右の入れ替え
    if (is_number(lhs->type) && is_pointer(rhs->type)) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        error_tok(tok, "ポインタ同士の加算はできません");
    }

    // lhs: pointer, rhs: number
    rhs = new_node_expr(ND_MUL, tok, rhs,
                        new_node_num(tok, get_sizeof(lhs->type->ptr_to)));
    return new_node_expr(ND_ADD, tok, lhs, rhs);
}

static Node *new_node_sub(Token *tok, Node *lhs, Node *rhs) {
    set_node_type(lhs);
    set_node_type(rhs);

    if (is_number(lhs->type) && is_number(rhs->type)) {
        return new_node_expr(ND_SUB, tok, lhs, rhs);
    }

    if (is_pointer(lhs->type) && is_number(rhs->type)) {
        rhs = new_node_expr(ND_MUL, tok, rhs,
                            new_node_num(tok, get_sizeof(lhs->type->ptr_to)));
        set_node_type(rhs);
        Node *node = new_node_expr(ND_SUB, tok, lhs, rhs);
        node->type = lhs->type;
        return node;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        Node *node = new_node_expr(ND_SUB, tok, lhs, rhs);
        node->type = new_type(TY_INT);
        return new_node_expr(ND_DIV, tok, node,
                             new_node_num(tok, get_sizeof(lhs->type->ptr_to)));
    }

    // lhs: number, rhs: pointer
    error_tok(tok, "誤ったオペランドです");
}

static Node *new_node_var(Token *tok) {
    Node *node = NULL;

    Object *var = find_var(tok);
    if (!var) {
        error_tok(tok, "宣言されていない変数です");
    }

    if (var->is_local) {
        node = new_node(ND_LVAR, tok);
    } else {
        node = new_node(ND_GVAR, tok);
    }
    node->var = var;

    return node;
}

// program = declaration*
Object *program() {
    while (!at_eof()) {
        declaration_global();
    }

    return globals->next;
}

// declspec = "int" | "char"
static Type *declspec() {
    Type *type = NULL;

    if (consume("int")) {
        type = new_type(TY_INT);
    } else if (consume("char")) {
        type = new_type(TY_CHAR);
    }

    return type;
}

static Type *declsuffix(Type *type) {
    if (consume("(")) {
        Type *base_type = type;

        type = new_type(TY_FUNC);
        type->ptr_to = base_type;
        return type;
    }

    if (consume("[")) {
        int len = expect_number();
        expect("]");
        type = declsuffix(type);
        return new_type_array(type, len);
    }

    return type;
}

// declarator = "*"* ident declsuffix
static Type *declarator(Type *type, Token **ident_tok) {
    while (consume("*")) {
        type = new_type_ptr(type);
    }

    *ident_tok = expect_ident();

    return declsuffix(type);
}

// declaration = declspec
//               (declarator ("=" assign)? ("," declarator ("=" assign)?)*)? ";"
static Node *declaration_local() {
    Type *base_type = declspec();
    if (base_type == NULL) {
        return NULL;
    }

    Token *tok = getok();
    Node head = {};
    Node *cur = &head;

    for (int i = 0; !consume(";"); i++) {
        if (i > 0) expect(",");

        Token *tok = NULL;
        Type *type = declarator(base_type, &tok);
        Object *var = new_lvar(type, tok);

        if (find_var_scope(tok) != NULL) {
            error_tok(tok, "再定義です");
        }
        add_lvar(var);

        if (consume("=")) {
            cur = cur->next = new_node(ND_EXPR_STMT, getok());
            cur->lhs = new_node_expr(ND_ASSIGN, getok()->prev,
                                     new_node_var(tok), assign());
        }
    }
    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;

    return node;
}

static void declaration_global() {
    Type *base_type = declspec();
    if (base_type == NULL) {
        error_tok(getok(), "型がありません");
    }

    for (int i = 0; !consume(";"); i++) {
        if (i > 0) expect(",");

        Token *tok = NULL;
        Type *type = declarator(base_type, &tok);

        if (type->kind == TY_FUNC) {
            function(type->ptr_to, tok);
            break;
        } else {
            // グローバル変数の再宣言は可能
            if (find_func(tok) != NULL) {
                error_tok(tok, "再定義です");
            }
            add_global(new_gvar(type, tok));
            if (consume("=")) {
                error_tok(getok()->prev, "初期化式は未対応です");
            }
        }
    }
}

static void function(Type *type, Token *tok) {
    if (find_func(tok) != NULL || find_var_scope(tok) != NULL) {
        error_tok(tok, "再定義です");
    }
    Object *func = new_func(type, tok);

    locals = NULL;

    enter_scope();

    params(func);
    func->body = stmt();
    func->locals = locals;

    leave_scope();

    add_global(func);
}

// params = (declare ident ("," declare ident)*)? ")"
static void params(Object *func) {
    if (consume(")")) return;

    Object head = {};
    Object *cur = &head;

    do {
        Type *base_type = declspec();
        if (base_type == NULL) {
            error_tok(getok()->prev, "型がありません");
        }

        Token *tok = NULL;
        Type *type = declarator(base_type, &tok);
        Object *var = new_lvar(type, tok);

        if (find_var_scope(tok) != NULL) {
            error_tok(tok, "引数の再定義");
        }
        push_scope(var);

        cur = cur->next = var;
        locals = head.next;
    } while (consume(","));
    func->params = head.next;
    expect(")");
}

// stmt = expr_stmt
//      | "{" compound_stmt
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
static Node *stmt() {
    if (consume("{")) {
        return compound_stmt();
    }

    if (consume("if")) {
        Node *node = new_node(ND_IF, getok()->prev);

        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }

        return node;
    }

    if (consume("while")) {
        Node *node = new_node(ND_WHILE, getok()->prev);

        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();

        return node;
    }

    if (consume("for")) {
        Node *node = new_node(ND_FOR, getok()->prev);

        expect("(");
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->upd = expr();
            expect(")");
        }
        node->then = stmt();

        return node;
    }

    if (consume("return")) {
        Node *node = new_node_expr(ND_RETURN, getok()->prev, expr(), NULL);
        expect(";");
        return node;
    }

    return expr_stmt();
}

// compound_stmt = (declaration_local | stmt)* "}"
static Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK, getok()->prev);

    Node head = {};
    Node *cur = &head;

    enter_scope();

    while (!consume("}")) {
        cur->next = declaration_local();
        if (!cur->next) {
            cur->next = stmt();
        }
        cur = cur->next;
        set_node_type(cur);
    }

    leave_scope();

    node->body = head.next;
    return node;
}

// expr_stmt = expr? ";"
static Node *expr_stmt() {
    if (consume(";")) {
        return new_node(ND_BLOCK, getok()->prev);
    }

    Node *node = new_node_expr(ND_EXPR_STMT, getok(), expr(), NULL);
    expect(";");
    return node;
}

// expr = assign ("," expr)?
static Node *expr() {
    Node *node = assign();
    if (consume(",")) {
        node = new_node_expr(ND_COMMA, getok()->prev, node, expr());
    }
    return node;
}

// assign = equality ("=" assign)?
static Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        node = new_node_expr(ND_ASSIGN, getok()->prev, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    while (true) {
        if (consume("==")) {
            node = new_node_expr(ND_EQ, getok()->prev, node, relational());
        } else if (consume("!=")) {
            node = new_node_expr(ND_NEQ, getok()->prev, node, relational());
        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
    Node *node = add();

    while (true) {
        if (consume("<=")) {
            node = new_node_expr(ND_LEQ, getok()->prev, node, add());
        } else if (consume("<")) {
            node = new_node_expr(ND_LS, getok()->prev, node, add());
        } else if (consume(">=")) {
            node = new_node_expr(ND_LEQ, getok()->prev, add(), node);
        } else if (consume(">")) {
            node = new_node_expr(ND_LS, getok()->prev, add(), node);
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add() {
    Node *node = mul();

    while (true) {
        if (consume("+")) {
            node = new_node_add(getok()->prev, node, mul());
        } else if (consume("-")) {
            node = new_node_sub(getok()->prev, node, mul());
        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();

    while (true) {
        if (consume("*")) {
            node = new_node_expr(ND_MUL, getok()->prev, node, unary());
        } else if (consume("/")) {
            node = new_node_expr(ND_DIV, getok()->prev, node, unary());
        } else {
            return node;
        }
    }
}

// unary = "sizeof" unary
//       | ("+" | "-" | "*" | "&") unary
//       | postfix
static Node *unary() {
    if (consume("sizeof")) {
        Node *node = unary();
        set_node_type(node);
        return new_node_num(getok()->prev, get_sizeof(node->type));
    }
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node_expr(ND_SUB, getok()->prev,
                             new_node_num(getok()->prev, 0), unary());
    }
    if (consume("*")) {
        return new_node_expr(ND_DEREF, getok()->prev, unary(), NULL);
    }
    if (consume("&")) {
        return new_node_expr(ND_ADDR, getok()->prev, unary(), NULL);
    }
    return postfix();
}

// postfix = primary ("[" num "]")*
static Node *postfix() {
    Node *lhs = primary();
    Node *rhs;

    while (consume("[")) {
        Token *tok = NULL;
        if ((tok = consume_ident())) {
            rhs = new_node_var(tok);
        } else {
            rhs = new_node_num(getok(), expect_number());
        }
        expect("]");

        tok = getok()->prev->prev->prev->prev;
        lhs = new_node_expr(ND_DEREF, tok, new_node_add(tok, lhs, rhs), NULL);
    }

    return lhs;
}

// callfunc = ident "(" (assign ("," assign)*)? ")"
static Node *callfunc(Token *tok) {
    Node *node = new_node(ND_CALL, tok);
    node->funcname = strndup(tok->loc, tok->len);

    if (!consume(")")) {
        Node head = {};
        Node *cur = &head;

        do {
            cur = cur->next = assign();
        } while (consume(","));
        expect(")");

        node->args = head.next;
    }

    return node;
}

static Node *string_literal(Token *tok) {
    Object *str_obj = new_string_literal(tok->str);
    str_obj->init_data = tok->str;
    add_global(str_obj);
    Node *node = new_node(ND_GVAR, tok);
    node->var = str_obj;
    return node;
}

// primary = num | ident | callfunc
//         | string | "(" expr ")"
//         | "(" "{" stmt+ "}" ")"
static Node *primary() {
    Token *tok;

    if (consume("(")) {
        Node *node;
        if (consume("{")) {
            node = new_node(ND_STMT_EXPR, getok()->prev);
            node->body = compound_stmt()->body;
        } else {
            node = expr();
        }
        expect(")");
        return node;
    }

    if ((tok = consume_ident())) {
        if (consume("(")) {
            return callfunc(tok);
        }

        return new_node_var(tok);
    }

    if ((tok = consume_string())) {
        return string_literal(tok);
    }

    return new_node_num(getok(), expect_number());
}
