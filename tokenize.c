#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

static Token *token;

Token *getok() { return token; }

void seek(Token *tok) { token = tok; }

// 次のトークンが期待している記号のときには、
// 真を返す。それ以外の場合には偽を返す。
bool match(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->loc, op, token->len)) {
        return false;
    }
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->loc, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->loc, op, token->len)) {
        error_tok(token, "'%s'ではありません", op);
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
        error_tok(token, "識別子ではありません");
    }
    Token *tmp = token;
    token = token->next;
    return tmp;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) {
        error_tok(token, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() { return token->kind == TK_EOF; }

bool equal(Token *tok, char *op) {
    return strlen(op) == tok->len && !memcmp(tok->loc, op, tok->len);
}

// 新しいトークンを作成して cur に繋げる
static Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = str;

    cur->next = tok;
    tok->prev = cur;

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
    static char *keywords[] = {"return", "if",     "else", "while",   "for",
                               "int",    "sizeof", "char", "struct",  "union",
                               "long",   "short",  "void", "typedef", "_Bool",
                               "enum",   "static"};
    static int len = sizeof(keywords) / sizeof(*keywords);
    for (int i = 0; i < len; i++) {
        if (equal(tok, keywords[i])) {
            return true;
        }
    }
    return false;
}

static int read_reserved(char *p) {
    static char *ops[] = {"==", "!=", "<=", ">=", "->"};
    static int len = sizeof(ops) / sizeof(*ops);
    for (int i = 0; i < len; i++) {
        if (startswith(p, ops[i])) {
            return strlen(ops[i]);
        }
    }
    if (strchr("+-*/()<>=;{},&*[].", *p)) {
        return 1;
    }
    return 0;
}

static int read_escaped_char(char **pos, char *p) {
    if ('0' <= *p && *p <= '7') {
        int c = *p++ - '0';
        if ('0' <= *p && *p <= '7') {
            c = (c << 3) + (*p++ - '0');
            if ('0' <= *p && *p <= '7') {
                c = (c << 3) + (*p++ - '0');
            }
        }
        *pos = p;
        return c;
    }

    if (*p == 'x') {
        p++;
        if (!isxdigit(*p)) {
            error_at(p, "16進数の数字ではありません");
        }
        int c = 0;
        for (; isxdigit(*p); p++) {
            c = (c << 4) + (*p - (isdigit(*p) ? '0' : 'a' - 10));
        }
        *pos = p;
        return c;
    }

    *pos = p + 1;
    switch (*p) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 't':
            return '\t';
        case 'n':
            return '\n';
        case 'v':
            return '\v';
        case 'f':
            return '\f';
        case 'r':
            return '\r';
        case 'e':
            return 27;
        default:
            return *p;
    }
}

static char *read_string_literal(char **pos, char *start) {
    char *p = start + 1;

    char *buf = NULL;
    size_t size = 0;

    FILE *fp = open_memstream(&buf, &size);

    while (*p != '"') {
        if (*p == '\n' || *p == '\0') {
            error_at(p, "文字列が閉じられていません");
        }

        if (*p == '\\') {
            fputc(read_escaped_char(&p, p + 1), fp);
        } else {
            fputc(*p++, fp);
        }
    }
    fflush(fp);
    fputc('\0', fp);
    fclose(fp);

    *pos = p + 1;
    return buf;
}

static char read_char_literal(char **pos, char *start) {
    char *p = start + 1;
    char c;

    while (*p != '\'') {
        if (*p == '\n' || *p == '\0') {
            error_at(p, "文字リテラルが閉じられていません");
        }

        if (*p == '\\') {
            c = read_escaped_char(&p, p + 1);
        } else {
            c = *p++;
        }
    }

    *pos = p + 1;
    return c;
}

static void add_line_nums(char *input, Token *tok) {
    char *p = input;
    int n = 1;

    do {
        if (p == tok->loc) {
            tok->line_no = n;
            tok = tok->next;
        }
        if (*p == '\n') {
            n++;
        }
    } while (*p++);
}

// 入力文字列 p をトークナイズする
void tokenize(char *p) {
    char *input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 行コメントをスキップ
        if (startswith(p, "//")) {
            p += 2;
            while (*p != '\n') {
                p++;
            }
            continue;
        }

        // ブロックコメントをスキップ
        if (startswith(p, "/*")) {
            char *q = strstr(p + 2, "*/");
            if (!q) {
                error_at(p, "コメントが閉じられていません");
            }
            p = q + 2;
            continue;
        }

        {
            int len = read_reserved(p);
            if (len > 0) {
                cur = new_token(TK_RESERVED, cur, p);
                cur->len = len;
                p += cur->len;
                continue;
            }
        }

        if (*p == '\'') {
            char *start = p;
            cur = new_token(TK_NUM, cur, p);
            cur->val = read_char_literal(&p, p);
            cur->len = p - start;
            continue;
        }

        if (*p == '"') {
            char *start = p;
            cur = new_token(TK_STR, cur, p);
            cur->str = read_string_literal(&p, p);
            cur->len = p - start;
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
    add_line_nums(input, head.next);
    token = head.next;
}
