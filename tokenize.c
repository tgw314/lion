#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%c'ではありません", op);
    }
    token = token->next;
}

Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *tmp = token;
    token = token->next;
    return tmp;
}

int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() { return token->kind == TK_EOF; }

Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;

    cur->next = tok;

    return tok;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        int len;

        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        len = 2;
        if (!strncmp(p, "==", len) || !strncmp(p, "!=", len) ||
            !strncmp(p, "<=", len) || !strncmp(p, ">=", len)) {
            cur = new_token(TK_RESERVED, cur, p);
            cur->len = len;
            p += len;
            continue;
        }

        len = 1;
        if (strchr("+-*/()<>=;", *p)) {
            cur = new_token(TK_RESERVED, cur, p);
            cur->len = len;
            p += len;
            continue;
        }
        if ('a' <= *p && *p <= 'z') {
            cur = new_token(TK_IDENT, cur, p);
            cur->len = len;
            p += len;
            continue;
        }

        len = 0;
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}
