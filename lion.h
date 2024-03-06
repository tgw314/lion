#include <stdbool.h>

// トークンの種類
typedef enum {
    TK_RESERVED,  // 記号
    TK_IDENT,     // 識別子
    TK_NUM,       // 数字
    TK_EOF,       // EOF
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind;  // トークンの型
    Token *next;     // 次の入力トークン
    int val;         // kind が TK_NUM の場合の数値
    char *str;       // トークン文字列
    int len;         // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_EQ,      // ==
    ND_NEQ,     // !=
    ND_LS,      // <
    ND_LEQ,     // <=
    ND_ASSIGN,  // =
    ND_LVAR,    // ローカル変数
    ND_NUM,     // 整数
    ND_RETURN,  // return
    ND_IF,      // if
    ND_ELSE,    // else
    ND_WHILE,   // while
    ND_FOR,     // for
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node *lhs;      // 左辺
    Node *rhs;      // 右辺
                    
    Node *cond;     // kind が ND_IF, ND_WHILE, ND_FOR の場合のみ
    Node *then;     // kind が ND_IF, ND_WHILE, ND_FOR の場合のみ
    Node *els;      // kind が ND_IF の場合のみ
    Node *init;     // kind が ND_FOR の場合のみ
    Node *upd;      // kind が ND_FOR の場合のみ
                    
    int val;        // kind が ND_NUM の場合の数値
    int offset;     /* kind が ND_LVAR の場合のみ
                       ローカル変数のベースポインタからのオフセット */
};

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
    LVar *next;  // 次の変数か NULL
    char *name;  // 変数の名前
    int len;     // 名前の長さ
    int offset;  // RBP からのオフセット
};

extern char *user_input;

extern Token *token;

extern Node *code[100];

extern LVar *locals;

// エラーを報告するための関数
// printf と同じ引数を取る
void error(char *fmt, ...);

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...);

bool consume(char *op);

Token *consume_kind(TokenKind kind);

void expect(char *op);

int expect_number();

bool at_eof();

bool equal(Token *tok, char *op);

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize(char *p);

void program();

void gen(Node *node);
