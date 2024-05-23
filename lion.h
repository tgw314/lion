#include <stdbool.h>
#include <stdint.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct Token Token;
typedef struct Node Node;
typedef struct Object Object;
typedef struct Type Type;
typedef struct Member Member;
typedef struct Relocation Relocation;

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
    ND_NULL_EXPR,  // 何もしない
    ND_ADD,        // +
    ND_SUB,        // -
    ND_MUL,        // *
    ND_DIV,        // /
    ND_MOD,        // %
    ND_EQ,         // ==
    ND_NEQ,        // !=
    ND_LS,         // <
    ND_LEQ,        // <=
    ND_AND,        // &&
    ND_OR,         // ||
    ND_BITAND,     // &
    ND_BITOR,      // |
    ND_BITXOR,     // ^
    ND_BITSHL,     // <<
    ND_BITSHR,     // >>
    ND_ASSIGN,     // =
    ND_COMMA,      // ,
    ND_COND,       // ? :
    ND_MEMBER,     // .
    ND_NEG,        // 単項 -
    ND_ADDR,       // 単項 &
    ND_DEREF,      // 単項 *
    ND_NOT,        // !
    ND_BITNOT,     // ~
    ND_CAST,       // キャスト
    ND_VAR,        // 変数
    ND_NUM,        // 整数
    ND_CALL,       // 関数呼び出し
    ND_STMT_EXPR,  // ステートメント式
    ND_EXPR_STMT,  // 式文
    ND_RETURN,     // return
    ND_IF,         // if
    ND_ELSE,       // else
    ND_FOR,        // for
    ND_BLOCK,      // ブロック
    ND_GOTO,       // goto
    ND_LABEL,      // ラベル
    ND_SWITCH,     // switch
    ND_CASE,       // case
    ND_DO,         // do
    ND_MEMZERO,    // スタックの変数をゼロ初期化
} NodeKind;

// トークン型
struct Token {
    TokenKind kind;  // トークンの型
    Token *prev;     // 前の入力トークン
    Token *next;     // 次の入力トークン
    Type *type;      // kind が TK_NUM, TK_STR の場合
    int64_t val;     // kind が TK_NUM の場合の数値
    char *str;       // kind が TK_STR の場合の文字列
    char *loc;       // トークン文字列
    int len;         // トークンの長さ
    int line_no;     // 行番号
};

// 型
struct Type {
    TypeKind kind;
    int size;
    int align;
    bool is_unsigned;
    Token *tok;

    Type *ptr_to;

    int array_size;

    Member *members;
    bool is_flexible;

    Type *return_type;
    Type *params;
    Type *next;
    bool is_variadic;
};

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;
    Token *tok;
    Type *type;
    Node *next;
    Node *lhs;
    Node *rhs;
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *upd;
    Node *body;
    int64_t val;
    Object *var;
    Type *functype;
    char *funcname;
    Node *args;
    Member *member;
    char *label;
    char *unique_label;
    char *break_label;
    char *continue_label;
    Node *goto_next;
    Node *case_next;
    Node *default_case;
};

struct Object {
    Object *next;
    char *name;
    Type *type;
    bool is_local;
    bool is_func;
    bool is_def;
    bool is_static;
    int align;

    // ローカル変数
    int offset;  // RBP からのオフセット

    // グローバル変数
    char *init_data;
    Relocation *rel;

    // 関数
    Object *locals;
    Object *params;
    Object *va_area;  // 可変長引数の領域
    Node *body;
};

// 構造体のメンバ
struct Member {
    Member *next;
    char *name;
    Type *type;
    int index;
    int offset;
    int align;
};

struct Relocation {
    Relocation *next;
    int offset;
    char *label;
    long addend;
};

// エラーを報告するための関数
// printf と同じ引数を取る
void error(char *fmt, ...);

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...);

void error_tok(Token *tok, char *fmt, ...);

#define unreachable() error("内部エラー: %s:%d", __FILE__, __LINE__)

Token *getok(void);

void seek(Token *tok);

bool match(char *op);

bool consume(char *op);

void expect(char *op);

Token *consume_ident(void);

Token *expect_ident(void);

int expect_number(void);

bool at_eof(void);

bool equal(Token *tok, char *op);

// 入力文字列 p をトークナイズしてそれを返す
void tokenize(char *p);

Node *node_cast(Token *tok, Type *type, Node *expr);

extern Type *type_void;
extern Type *type_bool;

extern Type *type_char;
extern Type *type_short;
extern Type *type_int;
extern Type *type_long;

extern Type *type_uchar;
extern Type *type_ushort;
extern Type *type_uint;
extern Type *type_ulong;

Type *type_func(Type *return_type, Type *params);
Type *type_enum(void);
Type *type_ptr(Type *type);
Type *type_array(Type *type, int size);
Type *type_struct(Member *members);
Type *type_union(Member *members);

Type *copy_type(Type *type);

bool is_pointer(Type *type);

bool is_integer(Type *type);

void set_node_type(Node *node);

Object *program(void);

int align(int n, int align);

void generate(Object *funcs);
