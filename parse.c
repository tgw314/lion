#include <stdlib.h>
#include <string.h>

#include "lion.h"

static Object *func_cur;

static Type *declspec();
static Type *declarator(Type *type, Token **ident_tok);

static Node *declaration();
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

// 変数を名前で検索する。見つからなかった場合は NULL を返す。
static Object *find_lvar(Token *tok) {
    for (Object *var = func_cur->locals; var; var = var->next) {
        if (var->name != NULL && !strncmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }

    return NULL;
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

static void add_lvar(Object *lvar) {
    static Object *cur = NULL;

    if (cur != NULL && cur->next == NULL) {
        cur->next = lvar;
        cur = cur->next;
        return;
    }
    for (cur = func_cur->locals; cur; cur = cur->next) {
        if (cur->next == NULL) {
            cur->next = lvar;
            cur = cur->next;
            return;
        }
    }
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

    if (lhs->type->kind == TY_INT && rhs->type->kind == TY_INT) {
        return new_node_expr(ND_ADD, lhs, rhs);
    }

    // 左右の入れ替え
    if (lhs->type->kind == TY_INT && rhs->type->kind == TY_PTR) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    if (lhs->type->kind == TY_PTR && rhs->type->kind == TY_PTR) {
        error("ポインタ同士の加算はできません");
    }

    // lhs: PTR, rhs: INT
    rhs =
        new_node_expr(ND_MUL, rhs, new_node_num(get_sizeof(lhs->type->ptr_to)));
    return new_node_expr(ND_ADD, lhs, rhs);
}

static Node *new_node_sub(Node *lhs, Node *rhs) {
    set_expr_type(lhs);
    set_expr_type(rhs);

    if (lhs->type->kind == TY_INT && rhs->type->kind == TY_INT) {
        return new_node_expr(ND_SUB, lhs, rhs);
    }

    if (lhs->type->kind == TY_PTR && rhs->type->kind == TY_INT) {
        rhs = new_node_expr(ND_MUL, rhs,
                            new_node_num(get_sizeof(lhs->type->ptr_to)));
        set_expr_type(rhs);
        Node *node = new_node_expr(ND_SUB, lhs, rhs);
        node->type = lhs->type;
        return node;
    }

    if (lhs->type->kind == TY_PTR && rhs->type->kind == TY_PTR) {
        Node *node = new_node_expr(ND_SUB, lhs, rhs);
        node->type = new_type(TY_INT);
        return new_node_expr(ND_DIV, node,
                             new_node_num(get_sizeof(lhs->type->ptr_to)));
    }

    error("誤ったオペランドです");
}

static Node *new_node_lvar(Token *tok) {
    Node *node = new_node(ND_LVAR);

    Object *lvar = find_lvar(tok);

    if (!lvar) {
        error_at(tok->str, "宣言されていない変数です");
    }

    node->lvar = lvar;

    return node;
}

// program = (declare ident "(" (declare ident ("," declare ident)*)? ")" stmt)*
Object *program() {
    Object func_head = {};
    func_cur = &func_head;

    while (!at_eof()) {
        {  // 関数名
            Type *base_type = declspec();
            if (base_type == NULL) {
                error("型がありません");
            }
            Token *tok = NULL;
            Type *type = declarator(base_type, &tok);

            if (type->kind == TY_FUNC) {
                type = type->ptr_to;

                func_cur->next = new_func(strndup(tok->str, tok->len));
                func_cur = func_cur->next;
            } else {
            }
        }

        Object locals_head = {};

        {  // 引数
            func_cur->locals = &locals_head;

            expect("(");
            if (!consume(")")) {
                while (true) {
                    Type *base_type = declspec();
                    if (base_type == NULL) {
                        error("型がありません");
                    }
                    Token *tok = NULL;
                    Type *type = declarator(base_type, &tok);

                    Object *param = new_lvar(type, strndup(tok->str, tok->len));

                    if (find_lvar(tok) != NULL) {
                        error_at(tok->str, "引数の再定義はできません");
                    }
                    add_lvar(param);
                    func_cur->param_count++;

                    if (!consume(",")) {
                        break;
                    }
                }
                expect(")");
            }
        }

        func_cur->body = stmt();
        func_cur->locals = locals_head.next;
    }
    return func_head.next;
}

// declspec = "int"
static Type *declspec() {
    Type *type = NULL;

    if (consume("int")) {
        type = new_type(TY_INT);
    }

    return type;
}

// declarator = "*"* ident ( ("[" num "]")* | "(" )?
static Type *declarator(Type *type, Token **ident_tok) {
    while (consume("*")) {
        type = new_type_ptr(type);
    }

    *ident_tok = expect_ident();

    while (consume("[")) {
        int len = expect_number();
        expect("]");
        type = new_type_array(type, len);
    }

    if (match("(")) {
        Type *base_type = type;

        type = new_type(TY_FUNC);
        type->ptr_to = base_type;
    }

    return type;
}

// declaration = declspec
//               (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration() {
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
            cur->next = new_node_expr(ND_ASSIGN, new_node_lvar(tok), expr());
            cur = cur->next;
        }
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;

    return node;
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
    } else if ((node = declaration())) {
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
//       | ("+" | "-")? primary
//       | ("*" | "&") unary
static Node *unary() {
    if (consume("sizeof")) {
        Node *node = unary();
        set_expr_type(node);
        return new_node_num(get_sizeof(node->type));
    }
    if (consume("+")) {
        return primary();
    }
    if (consume("-")) {
        return new_node_expr(ND_SUB, new_node_num(0), primary());
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

        node = new_node_lvar(tok);
        while (consume("[")) {
            int index = expect_number();
            expect("]");
            Node *lhs = new_node_add(node, new_node_num(index));

            node = new_node_expr(ND_DEREF, lhs, NULL);
        }

        return node;
    }

    return new_node_num(expect_number());
}
