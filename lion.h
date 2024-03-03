#include <stdbool.h>

// 入力プログラム
extern char *user_input;

// エラーを報告するための関数
// printf と同じ引数を取る
void error(char *fmt, ...);
// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...);

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

// 現在着目しているトークン
extern Token *token;

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op);

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op);

// 次のトークンが識別子のときには、トークンを1つ読み進めて
// そのトークンを返す。それ以外の場合には NULL を返す。
Token *consume_ident();

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number();

bool at_eof();

// 新しいトークンを作成して cur に繋げる
Token *new_token(TokenKind kind, Token *cur, char *str);

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize(char *p);

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
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node *lhs;      // 左辺
    Node *rhs;      // 右辺
    int val;        // kind が ND_NUM の場合の数値
    int offset;     /* kind が ND_LVAR の場合のみ
                       ローカル変数のベースポインタからのオフセット */
};

extern Node *code[100];

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
    LVar *next;  // 次の変数か NULL
    char *name;  // 変数の名前
    int len;     // 名前の長さ
    int offset;  // RBP からのオフセット
};

// ローカル変数
extern LVar *locals;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_lvar(Token *tok);

// 変数を名前で検索する。見つからなかった場合は NULL を返す。
LVar *find_lvar(Token *tok);

// program = stmt*
void program();

// stmt = expr ";"
Node *stmt();

// expr = assign
Node *expr();

// assign = equality ("=" assign)?
Node *assign();

// equality = relational ("==" relational | "!=" relational)*
Node *equality();

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational();

// add = mul ("+" mul | "-" mul)*
Node *add();

// mul = unary ("*" unary | "/" unary)*
Node *mul();

// unary = ("+" | "-")? primary
Node *unary();

// primary = num | ident | "(" expr ")"
Node *primary();

void gen(Node *node);
