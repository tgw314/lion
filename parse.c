#include <stdlib.h>
#include <string.h>

#include "lion.h"

// program = stmt*
void program();

// stmt = expr ";" | "return" expr ";"
static Node *stmt();

// expr = assign
static Node *expr();

// assign = equality ("=" assign)?
static Node *assign();

// equality = relational ("==" relational | "!=" relational)*
static Node *equality();

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational();

// add = mul ("+" mul | "-" mul)*
static Node *add();

// mul = unary ("*" unary | "/" unary)*
static Node *mul();

// unary = ("+" | "-")? primary
static Node *unary();

// primary = num | ident | "(" expr ")"
static Node *primary();

// 変数を名前で検索する。見つからなかった場合は NULL を返す。
static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }

    return NULL;
}

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

static Node *new_node_lvar(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar(tok);

    if (lvar) {
        node->offset = lvar->offset;
    } else {
        lvar = calloc(1, sizeof(LVar));

        lvar->next = locals;
        lvar->name = tok->str;
        lvar->len = tok->len;
        lvar->offset = ((locals) ? locals->offset : 0) + 8;

        node->offset = lvar->offset;
        locals = lvar;
    }

    return node;
}

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

static Node *stmt() {
    Node *node;

    if (consume_kind(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else {
        node = expr();
    }

    if (!consume(";")) {
        error_at(token->str, "';'ではないトークンです");
    }

    return node;
}

static Node *expr() { return assign(); }

static Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

static Node *equality() {
    Node *node = relational();

    while (true) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NEQ, node, relational());
        } else {
            return node;
        }
    }
}

static Node *relational() {
    Node *node = add();

    while (true) {
        if (consume("<=")) {
            node = new_node(ND_LEQ, node, add());
        } else if (consume("<")) {
            node = new_node(ND_LS, node, add());
        } else if (consume(">=")) {
            node = new_node(ND_LEQ, add(), node);
        } else if (consume(">")) {
            node = new_node(ND_LS, add(), node);
        } else {
            return node;
        }
    }
}

static Node *add() {
    Node *node = mul();

    while (true) {
        if (consume("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

static Node *mul() {
    Node *node = unary();

    while (true) {
        if (consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

static Node *unary() {
    if (consume("+")) {
        return primary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    }
    return primary();
}

static Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_kind(TK_IDENT);
    if (tok) {
        return new_node_lvar(tok);
    }

    return new_node_num(expect_number());
}
