#include <stdlib.h>
#include <string.h>

#include "lion.h"

static Object func_head = {};
static Object *func_cur;
static Object gvar_head = {};
static Object *gvar_cur;

static Type *declspec();
static Type *declarator(Type *type, Token **ident_tok);

static Node *declaration_local();
static void declaration_global();
static void params();
static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

static Object *new_func(char *name) {
    Object *func = calloc(1, sizeof(Object));
    func->name = name;
    func->is_func = true;
    func->is_local = false;
    return func;
}

static Object *new_gvar(Type *type, char *name) {
    Object *gvar = calloc(1, sizeof(Object));
    gvar->type = type;
    gvar->name = name;
    gvar->is_func = false;
    gvar->is_local = false;

    return gvar;
}

static Object *new_lvar(Type *type, char *name) {
    Object *lvar = calloc(1, sizeof(Object));
    lvar->type = type;
    lvar->name = name;
    lvar->is_func = false;
    lvar->is_local = true;

    func_cur->stack_size += get_sizeof(type);
    lvar->offset = func_cur->stack_size;

    return lvar;
}

// グローバル変数を名前で検索する。見つからなかった場合は NULL を返す。
static Object *find_gvar(Token *tok) {
    for (Object *var = gvar_head.next; var; var = var->next) {
        if (var->name != NULL && !strncmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }
    return NULL;
}

// ローカル変数を名前で検索する。見つからなかった場合は NULL を返す。
static Object *find_lvar(Token *tok) {
    for (Object *var = func_cur->locals; var; var = var->next) {
        if (var->name != NULL && !strncmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }

    return NULL;
}

static Object *find_var(Token *tok) {
    Object *lvar = find_lvar(tok);
    if (lvar) {
        return lvar;
    }
    return find_gvar(tok);
}

static Object *find_func(Token *tok) {
    for (Object *func = func_head.next; func; func = func->next) {
        if (func->name != NULL && !strncmp(tok->str, func->name, tok->len)) {
            return func;
        }
    }
    return NULL;
}

// lvar を func_cur->locals の末尾に追加する
static void add_lvar(Object *lvar) {
    static Object *cur = NULL;

    if (cur != NULL && cur->next == NULL) {
        cur->next = lvar;
        cur = cur->next;
        return;
    }
    for (cur = func_cur->locals; cur; cur = cur->next) {
        if (cur->next == NULL) {
            add_lvar(lvar);
            return;
        }
    }
}

static void add_gvar(Object *gvar) {
    gvar_cur->next = gvar;
    gvar_cur = gvar_cur->next;
}

static void add_func(Object *func) {
    func_cur->next = func;
    func_cur = func_cur->next;
}

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static Node *new_node_expr(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_add(Node *lhs, Node *rhs) {
    set_expr_type(lhs);
    set_expr_type(rhs);

    if (is_number(lhs->type) && is_number(rhs->type)) {
        return new_node_expr(ND_ADD, lhs, rhs);
    }

    // 左右の入れ替え
    if (is_number(lhs->type) && is_pointer(rhs->type)) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        error("ポインタ同士の加算はできません");
    }

    // lhs: pointer, rhs: number
    rhs =
        new_node_expr(ND_MUL, rhs, new_node_num(get_sizeof(lhs->type->ptr_to)));
    return new_node_expr(ND_ADD, lhs, rhs);
}

static Node *new_node_sub(Node *lhs, Node *rhs) {
    set_expr_type(lhs);
    set_expr_type(rhs);

    if (is_number(lhs->type) && is_number(rhs->type)) {
        return new_node_expr(ND_SUB, lhs, rhs);
    }

    if (is_pointer(lhs->type) && is_number(rhs->type)) {
        rhs = new_node_expr(ND_MUL, rhs,
                            new_node_num(get_sizeof(lhs->type->ptr_to)));
        set_expr_type(rhs);
        Node *node = new_node_expr(ND_SUB, lhs, rhs);
        node->type = lhs->type;
        return node;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        Node *node = new_node_expr(ND_SUB, lhs, rhs);
        node->type = new_type(TY_INT);
        return new_node_expr(ND_DIV, node,
                             new_node_num(get_sizeof(lhs->type->ptr_to)));
    }

    // lhs: number, rhs: pointer
    error("誤ったオペランドです");
}

static Node *new_node_var(Token *tok) {
    Node *node = NULL;

    Object *var = find_var(tok);
    if (!var) {
        error_at(tok->str, "宣言されていない変数です");
    }

    if (var->is_local) {
        node = new_node(ND_LVAR);
    } else {
        node = new_node(ND_GVAR);
    }
    node->var = var;

    return node;
}

// program = declaration*
Object *program() {
    func_cur = &func_head;
    gvar_cur = &gvar_head;

    while (!at_eof()) {
        declaration_global();
    }

    gvar_cur->next = func_head.next;
    return gvar_head.next;
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
//               (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration_local() {
    Type *base_type = declspec();
    if (base_type == NULL) {
        return NULL;
    }

    Node head = {};
    Node *cur = &head;

    for (int i = 0; !consume(";"); i++) {
        if (i > 0) expect(",");

        Token *tok = NULL;
        Type *type = declarator(base_type, &tok);
        Object *var = new_lvar(type, strndup(tok->str, tok->len));

        if (find_lvar(tok) != NULL) {
            error_at(tok->str, "再定義です");
        }
        add_lvar(var);

        if (consume("=")) {
            cur->next = new_node_expr(ND_ASSIGN, new_node_var(tok), expr());
            cur = cur->next;
        }
    }
    Node *node = new_node(ND_BLOCK);
    node->body = head.next;

    return node;
}

static void declaration_global() {
    Type *base_type = declspec();
    if (base_type == NULL) {
        error("型がありません");
    }

    for (int i = 0; !consume(";"); i++) {
        if (i > 0) expect(",");

        Token *tok = NULL;
        Type *type = declarator(base_type, &tok);

        if (type->kind == TY_FUNC) {
            Object locals_head = {};

            type = type->ptr_to;

            if (find_func(tok) != NULL || find_gvar(tok) != NULL) {
                error_at(tok->str, "再定義です");
            }
            add_func(new_func(strndup(tok->str, tok->len)));

            func_cur->locals = &locals_head;

            params();
            func_cur->body = stmt();

            func_cur->locals = locals_head.next;
            return;
        } else {
            // グローバル変数の再宣言は可能
            if (find_func(tok) != NULL) {
                error_at(tok->str, "再定義です");
            }
            add_gvar(new_gvar(type, strndup(tok->str, tok->len)));
        }

        if (consume("=")) {
            error("初期化式は未対応です");
        }
    }
}

// params = (declare ident ("," declare ident)*)? ")"
static void params() {
    if (consume(")")) return;

    do {
        Type *base_type = declspec();
        if (base_type == NULL) {
            error("型がありません");
        }

        Token *tok = NULL;
        Type *type = declarator(base_type, &tok);

        if (find_lvar(tok) != NULL) {
            error_at(tok->str, "引数の再定義");
        }
        add_lvar(new_lvar(type, strndup(tok->str, tok->len)));
        func_cur->param_count++;
    } while (consume(","));
    expect(")");
}

// stmt = expr? ";"
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | declaration
//      | "return" expr ";"
static Node *stmt() {
    Node *node;

    if (consume("{")) {
        node = new_node(ND_BLOCK);

        Node head = {};
        Node *cur = &head;
        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        cur->next = NULL;

        node->body = head.next;
        return node;
    } else if (consume("if")) {
        node = new_node(ND_IF);

        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }

        return node;
    } else if (consume("while")) {
        node = new_node(ND_WHILE);

        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();

        return node;
    } else if (consume("for")) {
        node = new_node(ND_FOR);

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
    } else if ((node = declaration_local())) {
        return node;
    }

    if (consume("return")) {
        node = new_node_expr(ND_RETURN, expr(), NULL);
    } else {
        if (consume(";")) {
            return new_node(ND_BLOCK);
        }
        node = expr();
    }

    expect(";");

    return node;
}

// expr = assign
static Node *expr() { return assign(); }

// assign = equality ("=" assign)?
static Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        node = new_node_expr(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    while (true) {
        if (consume("==")) {
            node = new_node_expr(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node_expr(ND_NEQ, node, relational());
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
            node = new_node_expr(ND_LEQ, node, add());
        } else if (consume("<")) {
            node = new_node_expr(ND_LS, node, add());
        } else if (consume(">=")) {
            node = new_node_expr(ND_LEQ, add(), node);
        } else if (consume(">")) {
            node = new_node_expr(ND_LS, add(), node);
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
            node = new_node_add(node, mul());
        } else if (consume("-")) {
            node = new_node_sub(node, mul());
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
            node = new_node_expr(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_node_expr(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

// unary = "sizeof" unary
//       | ("+" | "-" | "*" | "&") unary
static Node *unary() {
    if (consume("sizeof")) {
        Node *node = unary();
        set_expr_type(node);
        return new_node_num(get_sizeof(node->type));
    }
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node_expr(ND_SUB, new_node_num(0), unary());
    }
    if (consume("*")) {
        return new_node_expr(ND_DEREF, unary(), NULL);
    }
    if (consume("&")) {
        return new_node_expr(ND_ADDR, unary(), NULL);
    }
    return primary();
}

// primary = num
//         | ident ( "(" expr ("," expr)* ")" | "[" num "]" )?
//         | "(" expr ")"
static Node *primary() {
    Node *node;

    if (consume("(")) {
        node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        if (consume("(")) {
            node = new_node(ND_CALL);
            node->funcname = strndup(tok->str, tok->len);

            if (!consume(")")) {
                Node head = {};
                Node *cur = &head;

                cur->next = expr();
                cur = cur->next;
                while (!consume(")")) {
                    expect(",");

                    cur->next = expr();
                    cur = cur->next;
                }
                node->args = head.next;
            }

            return node;
        }

        node = new_node_var(tok);
        while (consume("[")) {
            int index = expect_number();
            expect("]");
            Node *lhs = new_node_add(node, new_node_num(index));

            node = new_node_expr(ND_DEREF, lhs, NULL);
        }

        return node;
    }

    node = new_node_num(expect_number());
    while (consume("[")) {
        Token *tok = expect_ident();
        expect("]");
        Node *lhs = new_node_add(node, new_node_var(tok));

        node = new_node_expr(ND_DEREF, lhs, NULL);
    }

    return node;
}
