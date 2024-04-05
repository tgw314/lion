#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    Object *var;
};

typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    char *name;
    Type *type;
};

typedef struct Scope Scope;
struct Scope {
    Scope *next;
    VarScope *vars;
    TagScope *tags;
};

static Object *locals;
static Object *globals = &(Object){};

static Scope *scope = &(Scope){};

static Type *declspec();
static Type *declarator(Type *type);
static Type *struct_decl();
static Type *union_decl();
static void declaration_global();
static void function(Type *type);
static Type *params();
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

static void push_tag_scope(Token *tok, Type *type) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->name = strndup(tok->loc, tok->len);
    sc->type = type;
    sc->next = scope->tags;
    scope->tags = sc;
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
    Type *type = new_type_array(num_type(TY_CHAR), strlen(str) + 1);
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

static Type *find_tag(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->next) {
        for (TagScope *sc2 = sc->tags; sc2; sc2 = sc2->next) {
            if (sc2->name != NULL && !strncmp(tok->loc, sc2->name, tok->len)) {
                return sc2->type;
            }
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

static Member *get_member(Type *type, Token *tok) {
    for (Member *mem = type->members; mem; mem = mem->next) {
        if (!strncmp(tok->loc, mem->name, tok->len)) {
            return mem;
        }
    }
    error_tok(tok, "存在しないメンバです");
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

static Node *new_node_num(Token *tok, int64_t val) {
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
                        new_node_num(tok, lhs->type->ptr_to->size));
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
                            new_node_num(tok, lhs->type->ptr_to->size));
        set_node_type(rhs);
        Node *node = new_node_expr(ND_SUB, tok, lhs, rhs);
        node->type = lhs->type;
        return node;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        Node *node = new_node_expr(ND_SUB, tok, lhs, rhs);
        node->type = num_type(TY_INT);
        return new_node_expr(ND_DIV, tok, node,
                             new_node_num(tok, lhs->type->ptr_to->size));
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

static bool is_decl() {
    return match("char") || match("short") || match("int") || match("long") ||
           match("struct") || match("union");
}

// declspec = "char" | "int" | "long" | "short"
//           | ("struct"|"union") struct-decl
static Type *declspec() {
    if (consume("char")) {
        return num_type(TY_CHAR);
    }
    if (consume("short")) {
        return num_type(TY_SHORT);
    }
    if (consume("int")) {
        return num_type(TY_INT);
    }
    if (consume("long")) {
        return num_type(TY_LONG);
    }
    if (consume("struct")) {
        return struct_decl();
    }
    if (consume("union")) {
        return union_decl();
    }

    error_tok(getok(), "型がありません");
}

static Type *declsuffix(Type *type) {
    if (consume("(")) {
        type = new_type_func(type, params());
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

// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) declsuffix
static Type *declarator(Type *type) {
    while (consume("*")) {
        type = new_type_ptr(type);
    }

    if (consume("(")) {
        Token *start = getok();
        declarator(&(Type){});
        expect(")");
        Type *base = declsuffix(type);
        Token *end = getok();
        seek(start);
        type = declarator(base);
        seek(end);
        return type;
    }

    Token *tok = expect_ident();
    type = declsuffix(type);
    type->tok = tok;

    return type;
}

// declaration = declspec
//               (declarator ("=" assign)? ("," declarator ("=" assign)?)*)? ";"
static Node *declaration_local() {
    Type *base_type = declspec();

    Token *start = getok();
    Node head = {};
    Node *cur = &head;

    for (int i = 0; !consume(";"); i++) {
        if (i > 0) expect(",");

        Type *type = declarator(base_type);
        Object *var = new_lvar(type, type->tok);

        if (find_var_scope(type->tok) != NULL) {
            error_tok(type->tok, "再定義です");
        }
        add_lvar(var);

        if (consume("=")) {
            cur = cur->next = new_node(ND_EXPR_STMT, getok());
            cur->lhs = new_node_expr(ND_ASSIGN, getok()->prev,
                                     new_node_var(type->tok), assign());
        }
    }
    Node *node = new_node(ND_BLOCK, start);
    node->body = head.next;

    return node;
}

static void declaration_global() {
    Type *base_type = declspec();

    for (int i = 0; !consume(";"); i++) {
        if (i > 0) expect(",");

        Type *type = declarator(base_type);

        if (type->kind == TY_FUNC && i == 0) {
            function(type);
            break;
        } else {
            // グローバル変数の再宣言は可能
            if (find_func(type->tok) != NULL) {
                error_tok(type->tok, "再定義です");
            }
            add_global(new_gvar(type, type->tok));
            if (consume("=")) {
                error_tok(getok()->prev, "初期化式は未対応です");
            }
        }
    }
}

static void add_params_lvar(Type *param) {
    if (param) {
        add_params_lvar(param->next);
        add_lvar(new_lvar(param, param->tok));
    }
}

static void function(Type *type) {
    Token *tok = type->tok;
    if (find_func(tok) != NULL || find_var_scope(tok) != NULL) {
        error_tok(tok, "再定義です");
    }
    Object *func = new_func(type, tok);

    locals = NULL;

    enter_scope();

    add_params_lvar(type->params);
    func->params = locals;
    func->body = stmt();
    func->locals = locals;

    leave_scope();

    add_global(func);
}

// params = (declare ident ("," declare ident)*)? ")"
static Type *params() {
    if (consume(")")) return NULL;

    Type head = {};
    Type *cur = &head;

    do {
        Type *base_type = declspec();
        Type *type = calloc(1, sizeof(Type));
        *type = *declarator(base_type);
        cur = cur->next = type;
    } while (consume(","));
    expect(")");

    return head.next;
}

// members = (declspec declarator ("," declarator)* ";")*
static Member *members() {
    Member head = {};
    Member *cur = &head;

    while (!consume("}")) {
        Type *base_type = declspec();

        for (int i = 0; !consume(";"); i++) {
            if (i > 0) expect(",");

            Member *mem = calloc(1, sizeof(Member));
            mem->type = declarator(base_type);
            mem->name = strndup(mem->type->tok->loc, mem->type->tok->len);
            cur = cur->next = mem;
        }
    }

    return head.next;
}

// struct-decl = ident? ("{" members)?
static Type *struct_decl() {
    Token *tok = consume_ident();
    if (tok && !match("{")) {
        Type *type = find_tag(tok);
        if (!type) {
            error_tok(tok, "不明な構造体です");
        }
        return type;
    }

    expect("{");

    Type *type = new_type_struct(members());
    if (tok) {
        push_tag_scope(tok, type);
    }

    return type;
}

// union-decl = ident? ("{" members)?
static Type *union_decl() {
    Token *tok = consume_ident();
    if (tok && !match("{")) {
        Type *type = find_tag(tok);
        if (!type) {
            error_tok(tok, "不明な共用体です");
        }
        return type;
    }

    expect("{");

    Type *type = new_type_union(members());
    if (tok) {
        push_tag_scope(tok, type);
    }

    return type;
}

static Node *struct_union_ref(Node *lhs) {
    set_node_type(lhs);
    if (lhs->type->kind != TY_STRUCT && lhs->type->kind != TY_UNION) {
        error_tok(lhs->tok, "構造体または共用体ではありません");
    };

    Token *tok = expect_ident();

    Node *node = new_node_expr(ND_MEMBER, tok, lhs, NULL);
    node->member = get_member(lhs->type, tok);

    return node;
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
        if (is_decl()) {
            cur->next = declaration_local();
        } else {
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
        return new_node_num(getok()->prev, node->type->size);
    }
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node_expr(ND_NEG, getok()->prev, unary(), NULL);
    }
    if (consume("*")) {
        return new_node_expr(ND_DEREF, getok()->prev, unary(), NULL);
    }
    if (consume("&")) {
        return new_node_expr(ND_ADDR, getok()->prev, unary(), NULL);
    }
    return postfix();
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
static Node *postfix() {
    Node *node = primary();

    while (true) {
        if (consume("[")) {
            Node *idx = expr();
            expect("]");

            Token *tok = getok()->prev;
            node = new_node_expr(ND_DEREF, tok, new_node_add(tok, node, idx),
                                 NULL);
            continue;
        }

        if (consume(".")) {
            node = struct_union_ref(node);
            continue;
        }

        if (consume("->")) {
            node = new_node_expr(ND_DEREF, getok()->prev, node, NULL);
            node = struct_union_ref(node);
            continue;
        }

        return node;
    }
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
