#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

static Token *token;

// 次のトークンが期待している記号のときには、
// 真を返す。それ以外の場合には偽を返す。
bool match(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        return false;
    }
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    token = token->next;
}

// 次のトークンの kind が TK_IDENT のときには、トークンを1つ読み進めて
// そのトークンを返す。それ以外の場合には NULL を返す。
Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *tmp = token;
    token = token->next;
    return tmp;
}

// 次のトークンの kind が TK_IDENT のときには、トークンを1つ読み進めて
// そのトークンを返す。それ以外の場合にはエラーを報告する。
Token *expect_ident() {
    if (token->kind != TK_IDENT) {
        error_at(token->str, "識別子ではありません");
    }
    Token *tmp = token;
    token = token->next;
    return tmp;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() { return token->kind == TK_EOF; }

static bool equal(Token *tok, char *op) {
    return strlen(op) == tok->len && !memcmp(tok->str, op, tok->len);
}

// 新しいトークンを作成して cur に繋げる
static Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;

    cur->next = tok;

    return tok;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

static bool is_al(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

static bool is_alnum(char c) { return is_al(c) || ('0' <= c && c <= '9'); }

static bool is_keyword(Token *tok) {
    static char *keywords[] = {"return", "if",  "else",  "while",
                               "for",    "int", "sizeof"};
    static int len = sizeof(keywords) / sizeof(*keywords);
    for (int i = 0; i < len; i++) {
        if (equal(tok, keywords[i])) {
            return true;
        }
    }
    return false;
}

// 入力文字列 p をトークナイズする
void tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") ||
            startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p);

            cur->len = 2;
            p += cur->len;
            continue;
        }

        if (strchr("+-*/()<>=;{},&*[]", *p)) {
            cur = new_token(TK_RESERVED, cur, p);

            cur->len = 1;
            p += cur->len;
            continue;
        }

        if (is_al(*p)) {
            cur = new_token(TK_IDENT, cur, p);

            cur->len = 1;
            while (is_alnum(*(p + cur->len))) cur->len++;

            if (is_keyword(cur)) {
                cur->kind = TK_RESERVED;
            }

            p += cur->len;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    token = head.next;
}
