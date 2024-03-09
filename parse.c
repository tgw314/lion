#include <stdlib.h>
#include <string.h>

#include "lion.h"

static Function *func_cur;

static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

static Function *new_func(char *name) {
    Function *func = calloc(1, sizeof(Function));
    func->name = name;
    return func;
}

// 変数を名前で検索する。見つからなかった場合は NULL を返す。
static LVar *find_lvar(LVar *lvars_head, Token *tok) {
    for (LVar *var = lvars_head; var; var = var->next) {
        if (var->name != NULL && !strncmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }

    return NULL;
}

static LVar *new_lvar(LVarKind kind, char *name, int len) {
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->kind = kind;
    lvar->name = strndup(name, len);
    lvar->offset = func_cur->stack_size + 8;
    func_cur->stack_size += 8;
    return lvar;
}

static void add_lvar(LVar *lvars_head, LVar *lvar) {
    for (LVar *var = lvars_head; var; var = var->next) {
        if (var->next == NULL) {
            var->next = lvar;
            return;
        }
    }
}

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_node_expr(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static Node *new_node_lvar(Token *tok) {
    Node *node = new_node(ND_LVAR);

    LVar *lvar = find_lvar(func_cur->locals, tok);

    if (!lvar) {
        lvar = new_lvar(LV_NORMAL, tok->str, tok->len);
        add_lvar(func_cur->locals, lvar);
    }

    node->offset = lvar->offset;

    return node;
}

// program = (ident "(" (ident ("," ident)*)? ")" stmt)*
Function *program() {
    Function func_head = {};
    func_cur = &func_head;

    while (!at_eof()) {
        {  // 関数名
            Token *tok = expect_ident();

            func_cur->next = new_func(strndup(tok->str, tok->len));
            func_cur = func_cur->next;
        }

        LVar locals_head = {};

        {  // 引数
            expect("(");
            func_cur->locals = &locals_head;
            Token *tok = consume_ident();
            if (tok) {
                add_lvar(func_cur->locals,
                         new_lvar(LV_ARG, tok->str, tok->len));
                while (!consume(")")) {
                    expect(",");
                    tok = expect_ident();
                    add_lvar(func_cur->locals,
                             new_lvar(LV_ARG, tok->str, tok->len));
                }
            } else {
                expect(")");
            }
        }

        func_cur->body = stmt();
        func_cur->locals = locals_head.next;
    }
    return func_head.next;
}

// stmt = expr ";"
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
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
    }

    if (consume("return")) {
        node = new_node(ND_RETURN);
        node->lhs = expr();
    } else {
        node = expr();
    }

    if (!consume(";")) {
        error_at(token->str, "';'ではないトークンです");
    }

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
            node = new_node_expr(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node_expr(ND_SUB, node, mul());
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

// unary = ("+" | "-")? primary
static Node *unary() {
    if (consume("+")) {
        return primary();
    }
    if (consume("-")) {
        return new_node_expr(ND_SUB, new_node_num(0), primary());
    }
    return primary();
}

// primary = num
//         | ident "(" (expr ("," expr)*)? ")"
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
        return new_node_lvar(tok);
    }

    return new_node_num(expect_number());
}
