#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    TY_VOID,
    TY_CHAR,
    TY_BOOL,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_ENUM,
    TY_PTR,
    TY_ARRAY,
    TY_FUNC,
    TY_STRUCT,
    TY_UNION,
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
    ND_NEG,        // 単項 '-'
    ND_ASSIGN,     // =
    ND_COMMA,      // ,
    ND_MEMBER,     // .
    ND_ADDR,       // &
    ND_DEREF,      // *
    ND_NOT,        // 論理否定
    ND_CAST,       // キャスト
    ND_VAR,        // 変数
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
    int64_t val;     // kind が TK_NUM の場合の数値
    char *str;       // kind が TK_STR の場合の文字列
    char *loc;       // トークン文字列
    int len;         // トークンの長さ
    int line_no;     // 行番号
};

// 型
struct Type {
    TypeKind kind;
    size_t size;
    int align;
    Token *tok;

    Type *ptr_to;
    size_t array_size;
    Member *members;

    Type *return_type;
    Type *params;
    Type *next;
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
    int64_t val;     // kind が ND_NUM の場合の数値
    Object *var;     // kind が ND_LVAR, ND_GVAR の場合のみ
    Type *functype;  // kind が ND_CALL の場合のみ
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
    bool is_def;
    bool is_static;

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

#define unreachable() error("内部エラー: %s:%d", __FILE__, __LINE__)

Token *getok();

void seek(Token *tok);

bool match(char *op);

bool consume(char *op);

void expect(char *op);

Token *consume_ident();

Token *expect_ident();

int expect_number();

bool at_eof();

bool equal(Token *tok, char *op);

// 入力文字列 p をトークナイズしてそれを返す
void tokenize(char *p);

Node *new_node_cast(Token *tok, Type *type, Node *expr);

Type *basic_type(TypeKind kind);

Type *new_type_func(Type *return_type, Type *params);

Type *new_type_enum();

Type *new_type_ptr(Type *type);

Type *new_type_array(Type *type, size_t size);

Type *new_type_struct(Member *member);

Type *new_type_union(Member *members);

bool is_pointer(Type *type);

bool is_number(Type *type);

void set_node_type(Node *node);

Object *program();

int align(int n, int align);

void generate(Object *funcs);
