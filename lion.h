#include <stdbool.h>
#include <stddef.h>

typedef struct Token Token;
typedef struct Node Node;
typedef struct Object Object;
typedef struct Type Type;
typedef struct Member Member;

// トークンの種類
typedef enum {
    TK_RESERVED,  // 記号
    TK_IDENT,     // 識別子
    TK_NUM,       // 数字
    TK_STR,       // 文字列
    TK_EOF,       // EOF
} TokenKind;

typedef enum {
    TY_INT,
    TY_CHAR,
    TY_PTR,
    TY_ARRAY,
    TY_FUNC,
    TY_STRUCT,
} TypeKind;

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,        // +
    ND_SUB,        // -
    ND_MUL,        // *
    ND_DIV,        // /
    ND_EQ,         // ==
    ND_NEQ,        // !=
    ND_LS,         // <
    ND_LEQ,        // <=
    ND_ASSIGN,     // =
    ND_COMMA,      // ,
    ND_MEMBER,     // .
    ND_ADDR,       // &
    ND_DEREF,      // *
    ND_LVAR,       // ローカル変数
    ND_GVAR,       // グローバル変数
    ND_NUM,        // 整数
    ND_CALL,       // 関数呼び出し
    ND_STMT_EXPR,  // ステートメント式
    ND_EXPR_STMT,  // 式文
    ND_RETURN,     // return
    ND_IF,         // if
    ND_ELSE,       // else
    ND_WHILE,      // while
    ND_FOR,        // for
    ND_BLOCK,      // ブロック
} NodeKind;

// トークン型
struct Token {
    TokenKind kind;  // トークンの型
    Token *prev;     // 前の入力トークン
    Token *next;     // 次の入力トークン
    int val;         // kind が TK_NUM の場合の数値
    char *str;       // kind が TK_STR の場合の文字列
    char *loc;       // トークン文字列
    int len;         // トークンの長さ
    int line_no;     // 行番号
};

// 型
struct Type {
    TypeKind kind;
    Type *ptr_to;
    size_t size;
    size_t array_size;
    int align;
    Member *members;
};

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;   // ノードの型
    Token *tok;      // トークン
    Type *type;      // 式の型
    Node *next;      // 次のステートメント
    Node *lhs;       // 左辺
    Node *rhs;       // 右辺
    Node *cond;      // kind が ND_IF, ND_WHILE, ND_FOR の場合のみ
    Node *then;      // kind が ND_IF, ND_WHILE, ND_FOR の場合のみ
    Node *els;       // kind が ND_IF の場合のみ
    Node *init;      // kind が ND_FOR の場合のみ
    Node *upd;       // kind が ND_FOR の場合のみ
    Node *body;      // kind が ND_BLOCK, ND_EXPR_STMT の場合のみ
    int val;         // kind が ND_NUM の場合の数値
    Object *var;     // kind が ND_LVAR, ND_GVAR の場合のみ
    char *funcname;  // kind が ND_CALL の場合のみ
    Node *args;      // kind が ND_CALL の場合のみ
    Member *member;  // kind が ND_MEMBER の場合のみ
};

// 関数
struct Object {
    char *name;
    Type *type;    // 返り値の型
    Object *next;  // 次の関数
    bool is_func;
    bool is_local;

    // ローカル変数
    int offset;  // RBP からのオフセット

    // グローバル変数
    char *init_data;

    // 関数
    int stack_size;
    Object *locals;  // ローカル変数
    Object *params;  // 引数
    Node *body;
};

// 構造体のメンバ
struct Member {
    Member *next;
    Type *type;
    char *name;
    int offset;
};

// エラーを報告するための関数
// printf と同じ引数を取る
void error(char *fmt, ...);

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...);

void error_tok(Token *tok, char *fmt, ...);

Token *getok();

bool match(char *op);

bool consume(char *op);

void expect(char *op);

Token *consume_ident();

Token *expect_ident();

Token *consume_string();

int expect_number();

bool at_eof();

// 入力文字列 p をトークナイズしてそれを返す
void tokenize(char *p);

Type *new_type_num(TypeKind kind);

Type *new_type_func();

Type *new_type_ptr(Type *type);

Type *new_type_array(Type *type, size_t size);

Type *new_type_struct(Member *member);

bool is_pointer(Type *type);

bool is_number(Type *type);

void set_node_type(Node *node);

Object *program();

int align(int n, int align);

void generate(Object *funcs);
