#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    char *name;
    Object *var;
    Type *type_def;
    Type *enum_type;
    int enum_val;
};

typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    char *name;
    Type *type;
};

typedef struct Scope Scope;
struct Scope {
    Scope *next;
    VarScope *vars;
    TagScope *tags;
};

typedef struct VarAttr VarAttr;
struct VarAttr {
    bool is_typedef;
    bool is_static;
};

typedef struct Initializer Initializer;
struct Initializer {
    Initializer *next;
    Type *type;
    Token *tok;
    Node *expr;              // 配列や構造体以外の型の場合
    Initializer **children;  // 配列や構造体の場合
};

typedef struct InitDesign InitDesign;
struct InitDesign {
    InitDesign *next;
    int index;
    Object *var;
};

static Object *locals;
static Object *globals = &(Object){};
static Object *current_func;

static Node *gotos;
static Node *labels;

static char *break_label;
static char *continue_label;

static Node *current_switch;

static Scope *scope = &(Scope){};

static Type *declspec(VarAttr *attr);
static Type *declarator(Type *type);
static Type *struct_union_decl(TypeKind kind);
static Type *struct_decl();
static Type *union_decl();
static Type *enum_specifier();
static void declaration_global(Type *base_type);
static Node *lvar_initializer(Object *var);
static void function(Type *type, VarAttr *attr);
static Type *params();
static void parse_typedef(Type *base_type);
static Node *stmt();
static Node *compound_stmt();
static Node *expr_stmt();
static Node *expr();
static int64_t const_expr();
static Node *assign();
static Node *conditional();
static Node *logor();
static Node *logand();
static Node *bit_or();
static Node *bitxor();
static Node *bitand();
static Node *equality();
static Node *shift();
static Node *relational();
static Node *add();
static Node *mul();
static Node *cast();
static Node *unary();
static Node *postfix();
static Node *primary();

static void enter_scope() {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->next = scope;
    scope = sc;
}

static void leave_scope() { scope = scope->next; }

static VarScope *push_scope(char *name) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->next = scope->vars;
    scope->vars = sc;
    return sc;
}

static void push_tag_scope(Token *tok, Type *type) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->name = strndup(tok->loc, tok->len);
    sc->type = type;
    sc->next = scope->tags;
    scope->tags = sc;
}

static char *unique_name() {
    static int index = 0;
    char *name = calloc(20, sizeof(char));
    sprintf(name, ".LC%d", index++);
    return name;
}

static Object *new_object(Type *type, char *name) {
    Object *var = calloc(1, sizeof(Object));
    var->type = type;
    var->name = name;
    return var;
}

static Object *new_func(Type *type, Token *tok) {
    Object *func = new_object(type, strndup(tok->loc, tok->len));
    func->is_local = false;
    func->is_func = true;
    return func;
}

static Object *new_gvar(Type *type, Token *tok) {
    Object *gvar = new_object(type, strndup(tok->loc, tok->len));
    gvar->is_local = false;
    gvar->is_func = false;
    return gvar;
}

static Object *new_anon_gvar(Type *type) {
    Object *gvar = new_object(type, unique_name());
    gvar->is_local = false;
    gvar->is_func = false;

    return gvar;
}

static Object *new_string_literal(char *str) {
    Type *type = new_type_array(basic_type(TY_CHAR), strlen(str) + 1);
    Object *gvar = new_anon_gvar(type);
    gvar->init_data = str;
    return gvar;
}

static Object *new_lvar(Type *type, Token *tok) {
    Object *lvar = new_object(type, strndup(tok->loc, tok->len));
    lvar->is_local = true;
    lvar->is_func = false;
    return lvar;
}

static Object *new_temp_lvar(Type *type) {
    Object *lvar = new_object(type, "");
    lvar->is_local = true;
    lvar->is_func = false;
    return lvar;
}

static Initializer *new_initializer(Type *type) {
    Initializer *init = calloc(1, sizeof(Initializer));
    init->type = type;

    if (type->kind == TY_ARRAY) {
        init->children = calloc(type->array_size, sizeof(Initializer *));
        for (int i = 0; i < type->array_size; i++) {
            init->children[i] = new_initializer(type->ptr_to);
        }
    }

    return init;
}

static VarScope *find_var(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->next) {
        for (VarScope *sc2 = sc->vars; sc2; sc2 = sc2->next) {
            if (equal(tok, sc2->name)) {
                return sc2;
            }
        }
    }
    return NULL;
}

static Type *find_tag(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->next) {
        for (TagScope *sc2 = sc->tags; sc2; sc2 = sc2->next) {
            if (equal(tok, sc2->name)) {
                return sc2->type;
            }
        }
    }
    return NULL;
}

static Object *find_func(Token *tok) {
    for (Object *func = globals; func; func = func->next) {
        if (func->is_func && equal(tok, func->name)) {
            return func;
        }
    }
    return NULL;
}

static Type *find_typedef(Token *tok) {
    if (tok->kind == TK_IDENT) {
        VarScope *sc = find_var(tok);
        if (sc) return sc->type_def;
    }
    return NULL;
}

static Member *get_member(Type *type, Token *tok) {
    for (Member *mem = type->members; mem; mem = mem->next) {
        if (equal(tok, mem->name)) {
            return mem;
        }
    }
    error_tok(tok, "存在しないメンバです");
}

static void check_var_redef(Token *tok) {
    for (VarScope *sc = scope->vars; sc; sc = sc->next) {
        if (equal(tok, sc->name)) {
            error_tok(tok, "再定義です");
        }
    }
}

static void check_func_redef(Token *tok) {
    if (find_func(tok)) {
        error_tok(tok, "再定義です");
    }
}

// ローカル変数を locals の先頭に追加する
static void add_lvar(Object *lvar) {
    lvar->next = locals;
    locals = lvar;
    push_scope(lvar->name)->var = lvar;
}

// 関数とグローバル変数を globals の末尾に追加する
static void add_global(Object *global) {
    static Object *cur = NULL;

    for (cur = globals; cur; cur = cur->next) {
        if (cur->next == NULL) {
            cur = cur->next = global;
            push_scope(global->name)->var = global;
            return;
        }
    }
}

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_node_num(Token *tok, int64_t val) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_node_long(Token *tok, int64_t val) {
    Node *node = new_node_num(tok, val);
    node->type = basic_type(TY_LONG);
    return node;
}

static Node *new_node_unary(NodeKind kind, Token *tok, Node *lhs) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

static Node *new_node_binary(NodeKind kind, Token *tok, Node *lhs, Node *rhs) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_add(Token *tok, Node *lhs, Node *rhs) {
    set_node_type(lhs);
    set_node_type(rhs);

    if (is_integer(lhs->type) && is_integer(rhs->type)) {
        return new_node_binary(ND_ADD, tok, lhs, rhs);
    }

    // 左右の入れ替え
    if (is_integer(lhs->type) && is_pointer(rhs->type)) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        error_tok(tok, "ポインタ同士の加算はできません");
    }

    // lhs: pointer, rhs: number
    rhs = new_node_binary(ND_MUL, tok, rhs,
                          new_node_long(tok, lhs->type->ptr_to->size));
    return new_node_binary(ND_ADD, tok, lhs, rhs);
}

static Node *new_node_sub(Token *tok, Node *lhs, Node *rhs) {
    set_node_type(lhs);
    set_node_type(rhs);

    if (is_integer(lhs->type) && is_integer(rhs->type)) {
        return new_node_binary(ND_SUB, tok, lhs, rhs);
    }

    if (is_pointer(lhs->type) && is_integer(rhs->type)) {
        rhs = new_node_binary(ND_MUL, tok, rhs,
                              new_node_long(tok, lhs->type->ptr_to->size));
        set_node_type(rhs);
        Node *node = new_node_binary(ND_SUB, tok, lhs, rhs);
        node->type = lhs->type;
        return node;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        Node *node = new_node_binary(ND_SUB, tok, lhs, rhs);
        node->type = basic_type(TY_INT);
        return new_node_binary(ND_DIV, tok, node,
                               new_node_num(tok, lhs->type->ptr_to->size));
    }

    // lhs: number, rhs: pointer
    error_tok(tok, "誤ったオペランドです");
}

static Node *new_node_var(Token *tok, Object *var) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

Node *new_node_cast(Token *tok, Type *type, Node *expr) {
    set_node_type(expr);

    Node *node = new_node(ND_CAST, tok);
    node->lhs = expr;
    node->type = calloc(1, sizeof(Type));
    *node->type = *type;
    return node;
}

static bool is_func() {
    Token *tok = getok();
    bool is_func = declarator(&(Type){})->kind == TY_FUNC;
    seek(tok);
    return is_func;
}

// program = (typedef | declaration_global)*
Object *program() {
    while (!at_eof()) {
        VarAttr attr = {};
        Type *base_type = declspec(&attr);

        if (attr.is_typedef) {
            parse_typedef(base_type);
            continue;
        }
        if (is_func()) {
            function(base_type, &attr);
            continue;
        }
        declaration_global(base_type);
    }

    return globals->next;
}

static bool is_decl() {
    static char *keywords[] = {"void",    "_Bool", "char",   "short",
                               "int",     "long",  "struct", "union",
                               "typedef", "enum",  "static"};
    static int len = sizeof(keywords) / sizeof(*keywords);
    for (int i = 0; i < len; i++) {
        if (match(keywords[i])) {
            return true;
        }
    }
    return find_typedef(getok());
}

// declspec = ("void" | "_Bool" | "char" | "int" | "long" | "short"
//             | "struct" struct-decl | "union" union-decl
//             | "typedef" | typedef-name | "enum" enum-specifier
//             | "static")+
static Type *declspec(VarAttr *attr) {
    enum {
        // clang-format off
        VOID  = 1 << 0,
        BOOL  = 1 << 2,
        CHAR  = 1 << 4,
        SHORT = 1 << 6,
        INT   = 1 << 8,
        LONG  = 1 << 10,
        OTHER = 1 << 12,
        // clang-format on
    };

    Type *type = basic_type(TY_INT);
    int counter = 0;

    while (is_decl()) {
        if (match("typedef") || match("static")) {
            Token *tok = getok();
            if (!attr) {
                error_tok(tok, "記憶クラス指定子は使用できません");
            }

            if (consume("typedef")) {
                attr->is_typedef = true;
            } else if (consume("static")) {
                attr->is_static = true;
            }

            if (attr->is_typedef && attr->is_static) {
                error_tok(tok, "`typedef`と`static`は併用できません");
            }
            continue;
        }

        Type *deftype = find_typedef(getok());
        if (match("struct") || match("union") || match("enum") || deftype) {
            if (counter) break;

            if (consume("struct")) {
                type = struct_union_decl(TY_STRUCT);
            } else if (consume("union")) {
                type = struct_union_decl(TY_UNION);
            } else if (consume("enum")) {
                type = enum_specifier();
            } else {
                type = deftype;
                seek(getok()->next);
            }

            counter += OTHER;
            continue;
        }

        if (consume("void")) {
            counter += VOID;
        } else if (consume("_Bool")) {
            counter += BOOL;
        } else if (consume("char")) {
            counter += CHAR;
        } else if (consume("short")) {
            counter += SHORT;
        } else if (consume("int")) {
            counter += INT;
        } else if (consume("long")) {
            counter += LONG;
        } else {
            unreachable();
        }

        switch (counter) {
            case VOID: type = basic_type(TY_VOID); break;
            case BOOL: type = basic_type(TY_BOOL); break;
            case CHAR: type = basic_type(TY_CHAR); break;
            case SHORT:
            case SHORT + INT: type = basic_type(TY_SHORT); break;
            case INT: type = basic_type(TY_INT); break;
            case LONG:
            case LONG + INT:
            case LONG + LONG:
            case LONG + LONG + INT: type = basic_type(TY_LONG); break;
            default: error_tok(getok()->prev, "不正な型です");
        }
    }

    return type;
}

// declsuffix = "(" params ")" |  "[" const_expr? "]" declsuffix
static Type *declsuffix(Type *type) {
    if (consume("(")) {
        type = new_type_func(type, params());
        return type;
    }

    if (consume("[")) {
        int len;

        if (consume("]")) {
            len = -1;
        } else {
            len = const_expr();
            expect("]");
        }

        type = declsuffix(type);
        return new_type_array(type, len);
    }

    return type;
}

// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) declsuffix
static Type *declarator(Type *type) {
    while (consume("*")) {
        type = new_type_ptr(type);
    }

    if (consume("(")) {
        Token *start = getok();
        declarator(&(Type){});
        expect(")");
        Type *base = declsuffix(type);
        Token *end = getok();
        seek(start);
        type = declarator(base);
        seek(end);
        return type;
    }

    Token *tok = expect_ident();
    type = declsuffix(type);
    type->tok = tok;

    return type;
}

// abstract-declarator = "*"* ("(" abstract-declarator ")")? declsuffix
static Type *abstract_declarator(Type *type) {
    while (consume("*")) {
        type = new_type_ptr(type);
    }

    if (consume("(")) {
        Token *start = getok();
        abstract_declarator(&(Type){});
        expect(")");
        Type *base = declsuffix(type);
        Token *end = getok();
        seek(start);
        type = abstract_declarator(base);
        seek(end);
        return type;
    }

    return declsuffix(type);
}

static Type *typename() { return abstract_declarator(declspec(NULL)); }

// declaration = declspec
//               (declarator ("=" assign)? ("," declarator ("=" assign)?)*)? ";"
static Node *declaration_local(Type *base_type) {
    Node *node = new_node(ND_BLOCK, getok());

    if (!consume(";")) {
        Node head = {};
        Node *cur = &head;

        do {
            Type *type = declarator(base_type);
            if (type->kind == TY_VOID) {
                error_tok(type->tok, "void 型の変数が宣言されました");
            }
            if (type->size < 0) {
                error_tok(type->tok, "不完全な型です");
            }

            check_var_redef(type->tok);

            Object *var = new_lvar(type, type->tok);
            add_lvar(var);

            if (consume("=")) {
                Token *tok = getok();
                Node *expr = lvar_initializer(var);
                cur = cur->next = new_node_unary(ND_EXPR_STMT, tok, expr);
            }
        } while (consume(","));
        expect(";");

        node->body = head.next;
    }

    return node;
}

static void declaration_global(Type *base_type) {
    if (consume(";")) return;

    do {
        Type *type = declarator(base_type);

        if (type->kind == TY_VOID) {
            error_tok(type->tok, "void 型の変数が宣言されました");
        }
        if (type->size < 0) {
            error_tok(type->tok, "不完全な型です");
        }

        // グローバル変数の再宣言は可能
        check_var_redef(type->tok);
        add_global(new_gvar(type, type->tok));

        if (consume("=")) {
            error_tok(getok()->prev, "初期化式は未対応です");
        }

    } while (consume(","));
    expect(";");
}

static void skip_excess_element() {
    if (consume("{")) {
        do {
            skip_excess_element();
        } while (consume(","));
        expect("}");
        return;
    }
    assign();
}

// initializer = "{" initializer ("," initializer)* "}"
//             | assign
static void parse_init(Initializer *init) {
    if (init->type->kind == TY_ARRAY) {
        expect("{");
        for (int i = 0; !consume("}"); i++) {
            if (i > 0) expect(",");

            if (i < init->type->array_size) {
                parse_init(init->children[i]);
            } else {
                skip_excess_element();
            }
        }
        return;
    }

    init->expr = assign();
}

static Initializer *initializer(Type *type) {
    Initializer *init = new_initializer(type);
    parse_init(init);
    return init;
}

static Node *init_design_expr(InitDesign *design) {
    Token *tok = getok();
    if (design->var) {
        return new_node_var(tok, design->var);
    }
    Node *lhs = init_design_expr(design->next);
    Node *rhs = new_node_num(tok, design->index);
    return new_node_unary(ND_DEREF, tok, new_node_add(tok, lhs, rhs));
}

static Node *create_lvar_init(Initializer *init, Type *type,
                              InitDesign *design) {
    Token *tok = getok();
    if (type->kind == TY_ARRAY) {
        Node *node = new_node(ND_NULL_EXPR, tok);

        for (int i = 0; i < type->array_size; i++) {
            InitDesign design2 = {design, i};
            Node *rhs =
                create_lvar_init(init->children[i], type->ptr_to, &design2);
            node = new_node_binary(ND_COMMA, tok, node, rhs);
        }

        return node;
    }

    if (!init->expr) return new_node(ND_NULL_EXPR, tok);

    Node *lhs = init_design_expr(design);
    Node *rhs = init->expr;
    return new_node_binary(ND_ASSIGN, tok, lhs, rhs);
}

static Node *lvar_initializer(Object *var) {
    Initializer *init = initializer(var->type);
    InitDesign desg = {NULL, 0, var};
    Token *tok = getok();

    Node *lhs = new_node(ND_MEMZERO, tok);
    lhs->var = var;

    Node *rhs = create_lvar_init(init, var->type, &desg);

    return new_node_binary(ND_COMMA, tok, lhs, rhs);
}

static void add_params_lvar(Type *param) {
    if (param) {
        add_params_lvar(param->next);
        add_lvar(new_lvar(param, param->tok));
    }
}

static void resolve_goto_labels() {
    for (Node *jmp = gotos; jmp; jmp = jmp->goto_next) {
        for (Node *decl = labels; decl; decl = decl->goto_next) {
            if (!strcmp(jmp->label, decl->label)) {
                jmp->unique_label = decl->unique_label;
                break;
            }
        }

        if (jmp->unique_label == NULL) {
            error_tok(jmp->tok->next, "このラベルは宣言されていません");
        }
    }

    gotos = labels = NULL;
}

static void function(Type *base_type, VarAttr *attr) {
    Type *type = declarator(base_type);
    Token *tok = type->tok;

    check_var_redef(tok);
    check_func_redef(tok);

    Object *func = new_func(type, tok);
    func->is_def = !consume(";");
    func->is_static = attr->is_static;
    add_global(func);

    if (!func->is_def) {
        return;
    }

    locals = NULL;
    current_func = func;

    enter_scope();

    add_params_lvar(type->params);
    func->params = locals;
    func->body = stmt();
    func->locals = locals;

    leave_scope();
    resolve_goto_labels();
}

// params = (declare ident ("," declare ident)*)? ")"
static Type *params() {
    if (consume(")")) return NULL;

    Type head = {};
    Type *cur = &head;

    do {
        Type *type = calloc(1, sizeof(Type));
        *type = *declarator(declspec(NULL));
        if (type->kind == TY_ARRAY) {
            Token *tok = type->tok;
            type = new_type_ptr(type->ptr_to);
            type->tok = tok;
        }
        cur = cur->next = type;
    } while (consume(","));
    expect(")");

    return head.next;
}

// members = (declspec declarator ("," declarator)* ";")*
static Member *members() {
    Member head = {};
    Member *cur = &head;

    while (!consume("}")) {
        if (consume(";")) continue;

        Type *base_type = declspec(NULL);
        do {
            Member *mem = calloc(1, sizeof(Member));
            mem->type = declarator(base_type);
            mem->name = strndup(mem->type->tok->loc, mem->type->tok->len);
            cur = cur->next = mem;
        } while (consume(","));
        expect(";");
    }

    return head.next;
}

// struct-decl = ident? ("{" members)?
static Type *struct_union_decl(TypeKind kind) {
    if (kind != TY_STRUCT && kind != TY_UNION) {
        unreachable();
    }

    Token *tag = consume_ident();
    if (tag && !match("{")) {
        Type *type = find_tag(tag);

        if (type) {
            if (type->kind != kind) {
                if (kind == TY_STRUCT) {
                    error_tok(tag, "構造体のタグではありません");
                } else {
                    error_tok(tag, "共用体のタグではありません");
                }
            }
            return type;
        }

        type = new_type_struct_union(kind, NULL);
        type->size = -1;
        push_tag_scope(tag, type);
        return type;
    }

    expect("{");

    Type *type = new_type_struct_union(kind, members());
    if (tag) {
        for (TagScope *sc = scope->tags; sc; sc = sc->next) {
            if (equal(tag, sc->name)) {
                *sc->type = *type;
                return sc->type;
            }
        }
        push_tag_scope(tag, type);
    }

    return type;
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident ("{" enum-list? "}")?
//
// enum-list      = ident ("=" const_expr)? ("," ident ("=" const_expr)?)*
static Type *enum_specifier() {
    Token *tag = consume_ident();
    if (tag && !match("{")) {
        Type *type = find_tag(tag);
        if (!type) {
            error_tok(tag, "不明な列挙型です");
        }
        if (type->kind != TY_ENUM) {
            error_tok(tag, "列挙型のタグではありません");
        }
        return type;
    }

    expect("{");

    Type *type = new_type_enum();
    if (!consume("}")) {
        int val = 0;
        do {
            Token *tok = expect_ident();
            char *name = strndup(tok->loc, tok->len);

            if (consume("=")) {
                val = const_expr();
            }

            VarScope *sc = push_scope(name);
            sc->enum_type = type;
            sc->enum_val = val++;
        } while (consume(","));
        expect("}");
    }

    if (tag) {
        push_tag_scope(tag, type);
    }

    return type;
}

static Node *struct_union_ref(Node *lhs) {
    set_node_type(lhs);
    if (lhs->type->kind != TY_STRUCT && lhs->type->kind != TY_UNION) {
        error_tok(lhs->tok, "構造体または共用体ではありません");
    };

    Token *tok = expect_ident();

    Node *node = new_node_unary(ND_MEMBER, tok, lhs);
    node->member = get_member(lhs->type, tok);

    return node;
}

static void parse_typedef(Type *base_type) {
    if (consume(";")) return;

    do {
        Type *type = declarator(base_type);
        char *name = strndup(type->tok->loc, type->tok->len);
        check_var_redef(type->tok);
        push_scope(name)->type_def = type;
    } while (consume(","));
    expect(";");
}

// stmt = expr_stmt
//      | "{" compound_stmt
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "goto" ident ";"
//      | ident ":" stmt
//      | "break" ";"
//      | "continue" ";"
//      | "switch" "(" expr ")" stmt
//      | (case const_expr | "default") ":" stmt
//      | "return" expr ";"
static Node *stmt() {
    Token *tok = getok();
    if (consume("{")) {
        return compound_stmt();
    }

    if (consume("if")) {
        Node *node = new_node(ND_IF, tok);

        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }

        return node;
    }

    if (consume("while")) {
        Node *node = new_node(ND_FOR, tok);

        expect("(");
        node->cond = expr();
        expect(")");

        char *brk = break_label;
        char *cont = continue_label;
        break_label = node->break_label = unique_name();
        continue_label = node->continue_label = unique_name();

        node->then = stmt();

        break_label = brk;
        continue_label = cont;

        return node;
    }

    if (consume("for")) {
        Node *node = new_node(ND_FOR, tok);

        expect("(");

        enter_scope();

        char *brk = break_label;
        char *cont = continue_label;
        break_label = node->break_label = unique_name();
        continue_label = node->continue_label = unique_name();

        if (is_decl()) {
            node->init = declaration_local(declspec(NULL));
        } else {
            node->init = expr_stmt();
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

        leave_scope();

        break_label = brk;
        continue_label = cont;

        return node;
    }

    if (consume("goto")) {
        Node *node = new_node(ND_GOTO, tok);
        Token *label = expect_ident();
        expect(";");

        node->label = strndup(label->loc, label->len);
        node->goto_next = gotos;
        gotos = node;

        return node;
    }

    if (tok->kind == TK_IDENT) {
        seek(tok->next);
        if (consume(":")) {
            Node *node = new_node_unary(ND_LABEL, tok, stmt());

            node->label = strndup(tok->loc, tok->len);
            node->unique_label = unique_name();
            node->goto_next = labels;
            labels = node;

            return node;
        }
        seek(tok);
    }

    if (consume("break")) {
        if (!break_label) {
            error_tok(tok, "break 文はここでは使えません");
        }
        Node *node = new_node(ND_GOTO, tok);
        node->unique_label = break_label;
        expect(";");
        return node;
    }

    if (consume("continue")) {
        if (!continue_label) {
            error_tok(tok, "continue 文はここでは使えません");
        }
        Node *node = new_node(ND_GOTO, tok);
        node->unique_label = continue_label;
        expect(";");
        return node;
    }

    if (consume("switch")) {
        Node *node = new_node(ND_SWITCH, tok);
        expect("(");
        node->cond = expr();
        expect(")");

        Node *sw = current_switch;
        char *brk = break_label;
        current_switch = node;
        break_label = node->break_label = unique_name();

        node->then = stmt();

        current_switch = sw;
        break_label = brk;
        return node;
    }

    if (consume("case")) {
        if (!current_switch) {
            error_tok(tok, "case 文はここでは使えません");
        }
        int val = const_expr();
        expect(":");

        Node *node = new_node_unary(ND_CASE, tok, stmt());
        node->label = unique_name();
        node->val = val;
        node->case_next = current_switch->case_next;
        current_switch->case_next = node;
        return node;
    }

    if (consume("default")) {
        if (!current_switch) {
            error_tok(tok, "default 文はここでは使えません");
        }
        expect(":");

        Node *node = new_node_unary(ND_CASE, tok, stmt());
        node->label = unique_name();
        current_switch->default_case = node;
        return node;
    }

    if (consume("return")) {
        Node *node = expr();
        expect(";");

        set_node_type(node);
        node = new_node_cast(node->tok, current_func->type->return_type, node);
        node = new_node_unary(ND_RETURN, tok, node);
        return node;
    }

    return expr_stmt();
}

// compound_stmt = (typedef | declaration_local | stmt)* "}"
static Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK, getok()->prev);

    Node head = {};
    Node *cur = &head;

    enter_scope();

    while (!consume("}")) {
        if (is_decl() && !equal(getok()->next, ":")) {
            VarAttr attr = {};
            Type *base_type = declspec(&attr);

            if (attr.is_typedef) {
                parse_typedef(base_type);
                continue;
            }

            cur->next = declaration_local(base_type);
        } else {
            cur->next = stmt();
        }
        cur = cur->next;
        set_node_type(cur);
    }

    leave_scope();

    node->body = head.next;
    return node;
}

// expr_stmt = expr? ";"
static Node *expr_stmt() {
    Token *tok = getok();
    if (consume(";")) {
        return new_node(ND_BLOCK, tok);
    }

    Node *node = new_node_unary(ND_EXPR_STMT, tok, expr());
    expect(";");
    return node;
}

// expr = assign ("," expr)?
static Node *expr() {
    Node *node = assign();
    if (consume(",")) {
        node = new_node_binary(ND_COMMA, getok()->prev, node, expr());
    }
    return node;
}

static int64_t eval(Node *node) {
    set_node_type(node);
    switch (node->kind) {
        case ND_ADD: return eval(node->lhs) + eval(node->rhs);
        case ND_SUB: return eval(node->lhs) - eval(node->rhs);
        case ND_MUL: return eval(node->lhs) * eval(node->rhs);
        case ND_DIV: return eval(node->lhs) / eval(node->rhs);
        case ND_MOD: return eval(node->lhs) % eval(node->rhs);
        case ND_EQ: return eval(node->lhs) == eval(node->rhs);
        case ND_NEQ: return eval(node->lhs) != eval(node->rhs);
        case ND_LS: return eval(node->lhs) < eval(node->rhs);
        case ND_LEQ: return eval(node->lhs) <= eval(node->rhs);
        case ND_AND: return eval(node->lhs) && eval(node->rhs);
        case ND_OR: return eval(node->lhs) || eval(node->rhs);
        case ND_BITAND: return eval(node->lhs) & eval(node->rhs);
        case ND_BITOR: return eval(node->lhs) | eval(node->rhs);
        case ND_BITXOR: return eval(node->lhs) ^ eval(node->rhs);
        case ND_BITSHL: return eval(node->lhs) << eval(node->rhs);
        case ND_BITSHR: return eval(node->lhs) >> eval(node->rhs);
        case ND_COMMA: return eval(node->rhs);
        case ND_COND:
            return eval(node->cond) ? eval(node->then) : eval(node->els);
        case ND_NEG: return -eval(node->lhs);
        case ND_NOT: return !eval(node->lhs);
        case ND_BITNOT: return ~eval(node->lhs);
        case ND_CAST:
            if (is_integer(node->type)) {
                switch (node->type->size) {
                    case 1: return (uint8_t)eval(node->lhs);
                    case 2: return (uint16_t)eval(node->lhs);
                    case 4: return (uint32_t)eval(node->lhs);
                }
            }
            return eval(node->lhs);
        case ND_NUM: return node->val;
    }

    error_tok(node->tok, "コンパイル時に定数ではありません");
}

static int64_t const_expr() { return eval(conditional()); }

static Node *to_assign(Node *binary) {
    set_node_type(binary->lhs);
    set_node_type(binary->rhs);

    Token *tok = binary->tok;
    Object *var = new_temp_lvar(new_type_ptr(binary->lhs->type));
    add_lvar(var);

    Node *expr1, *expr2;

    expr1 = new_node_unary(ND_ADDR, tok, binary->lhs);
    expr1 = new_node_binary(ND_ASSIGN, tok, new_node_var(tok, var), expr1);

    expr2 = new_node_unary(ND_DEREF, tok, new_node_var(tok, var));
    expr2 = new_node_binary(binary->kind, tok, expr2, binary->rhs);
    expr2 = new_node_binary(
        ND_ASSIGN, tok, new_node_unary(ND_DEREF, tok, new_node_var(tok, var)),
        expr2);

    return new_node_binary(ND_COMMA, tok, expr1, expr2);
}

// assign = conditional (
//            ("=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" |
//            "<<=" | ">>=") assign
//          )?
static Node *assign() {
    Token *tok = getok();

    Node *node = conditional();
    if (consume("=")) {
        node = new_node_binary(ND_ASSIGN, tok, node, assign());
    }
    if (consume("+=")) {
        node = to_assign(new_node_add(tok, node, assign()));
    }
    if (consume("-=")) {
        node = to_assign(new_node_sub(tok, node, assign()));
    }
    if (consume("*=")) {
        node = to_assign(new_node_binary(ND_MUL, tok, node, assign()));
    }
    if (consume("/=")) {
        node = to_assign(new_node_binary(ND_DIV, tok, node, assign()));
    }
    if (consume("%=")) {
        node = to_assign(new_node_binary(ND_MOD, tok, node, assign()));
    }
    if (consume("&=")) {
        node = to_assign(new_node_binary(ND_BITAND, tok, node, assign()));
    }
    if (consume("|=")) {
        node = to_assign(new_node_binary(ND_BITOR, tok, node, assign()));
    }
    if (consume("^=")) {
        node = to_assign(new_node_binary(ND_BITXOR, tok, node, assign()));
    }
    if (consume("<<=")) {
        node = to_assign(new_node_binary(ND_BITSHL, tok, node, assign()));
    }
    if (consume(">>=")) {
        node = to_assign(new_node_binary(ND_BITSHR, tok, node, assign()));
    }
    return node;
}

// conditional = logor ("?" expr ":" conditional)?
static Node *conditional() {
    Token *tok = getok();

    Node *cond = logor();

    if (!consume("?")) return cond;

    Node *node = new_node(ND_COND, tok);
    node->cond = cond;
    node->then = expr();
    expect(":");
    node->els = conditional();

    return node;
}

// logor = logand ("||" logand)*
static Node *logor() {
    Node *node = logand();
    while (consume("||")) {
        node = new_node_binary(ND_OR, getok()->prev, node, logand());
    }
    return node;
}

// logand = bitor ("&&" bitor)*
static Node *logand() {
    Node *node = bit_or();
    while (consume("&&")) {
        node = new_node_binary(ND_AND, getok()->prev, node, bit_or());
    }
    return node;
}

// bitor = bitxor ("|" bitxor)*
static Node *bit_or() {
    Node *node = bitxor();
    while (consume("|")) {
        node = new_node_binary(ND_BITOR, getok()->prev, node, bitxor());
    }
    return node;
}

// bitxor = bitand ("^" bitand)*
static Node *bitxor() {
    Node *node = bitand();
    while (consume("^")) {
        node = new_node_binary(ND_BITXOR, getok()->prev, node, bitand());
    }
    return node;
}
// bitand = equality ("&" equality)*
static Node *bitand() {
    Node *node = equality();
    while (consume("&")) {
        node = new_node_binary(ND_BITAND, getok()->prev, node, equality());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    while (true) {
        Token *tok = getok();
        if (consume("==")) {
            node = new_node_binary(ND_EQ, tok, node, relational());
        } else if (consume("!=")) {
            node = new_node_binary(ND_NEQ, tok, node, relational());
        } else {
            return node;
        }
    }
}

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static Node *relational() {
    Node *node = shift();

    while (true) {
        Token *tok = getok();
        if (consume("<=")) {
            node = new_node_binary(ND_LEQ, tok, node, shift());
        } else if (consume("<")) {
            node = new_node_binary(ND_LS, tok, node, shift());
        } else if (consume(">=")) {
            node = new_node_binary(ND_LEQ, tok, shift(), node);
        } else if (consume(">")) {
            node = new_node_binary(ND_LS, tok, shift(), node);
        } else {
            return node;
        }
    }
}

// shift = add ("<<" add | ">>" add)
static Node *shift() {
    Node *node = add();

    while (true) {
        Token *tok = getok();
        if (consume("<<")) {
            node = new_node_binary(ND_BITSHL, tok, node, add());
        } else if (consume(">>")) {
            node = new_node_binary(ND_BITSHR, tok, node, add());
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add() {
    Node *node = mul();

    while (true) {
        Token *tok = getok();
        if (consume("+")) {
            node = new_node_add(tok, node, mul());
        } else if (consume("-")) {
            node = new_node_sub(tok, node, mul());
        } else {
            return node;
        }
    }
}

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node *mul() {
    Node *node = cast();

    while (true) {
        Token *tok = getok();
        if (consume("*")) {
            node = new_node_binary(ND_MUL, tok, node, cast());
        } else if (consume("/")) {
            node = new_node_binary(ND_DIV, tok, node, cast());
        } else if (consume("%")) {
            node = new_node_binary(ND_MOD, tok, node, cast());
        } else {
            return node;
        }
    }
}

// cast = "(" typename ")" cast | unary
static Node *cast() {
    Token *tok = getok();
    if (consume("(") && is_decl()) {
        Type *type = typename();
        expect(")");
        return new_node_cast(tok, type, cast());
    }

    seek(tok);
    return unary();
}

// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
static Node *unary() {
    Token *tok = getok();
    if (consume("+")) {
        return cast();
    }
    if (consume("-")) {
        return new_node_unary(ND_NEG, tok, cast());
    }
    if (consume("*")) {
        return new_node_unary(ND_DEREF, tok, cast());
    }
    if (consume("&")) {
        return new_node_unary(ND_ADDR, tok, cast());
    }
    if (consume("!")) {
        return new_node_unary(ND_NOT, tok, cast());
    }
    if (consume("~")) {
        return new_node_unary(ND_BITNOT, tok, cast());
    }
    if (consume("++")) {
        return to_assign(new_node_add(tok, unary(), new_node_num(tok, 1)));
    }
    if (consume("--")) {
        return to_assign(new_node_sub(tok, unary(), new_node_num(tok, 1)));
    }
    return postfix();
}

// postfix = primary ("[" expr "]" | ("." | "->") ident | "++" | "--")*
static Node *postfix() {
    Node *node = primary();

    while (true) {
        Token *tok = getok();
        if (consume("[")) {
            Node *idx = expr();
            expect("]");

            node = new_node_unary(ND_DEREF, tok, new_node_add(tok, node, idx));
            continue;
        }

        if (consume(".")) {
            node = struct_union_ref(node);
            continue;
        }

        if (consume("->")) {
            node = new_node_unary(ND_DEREF, tok, node);
            node = struct_union_ref(node);
            continue;
        }

        if (consume("++")) {
            set_node_type(node);
            Type *type = node->type;

            node = to_assign(new_node_add(tok, node, new_node_num(tok, 1)));
            node = new_node_sub(tok, node, new_node_num(tok, 1));
            node = new_node_cast(tok, type, node);
            continue;
        }

        if (consume("--")) {
            set_node_type(node);
            Type *type = node->type;

            node = to_assign(new_node_sub(tok, node, new_node_num(tok, 1)));
            node = new_node_add(tok, node, new_node_num(tok, 1));
            node = new_node_cast(tok, type, node);
            continue;
        }

        return node;
    }
}

// callfunc = ident "(" (assign ("," assign)*)? ")"
static Node *callfunc(Token *tok) {
    VarScope *sc = find_var(tok);

    if (!sc) error_tok(tok, "関数の暗黙的な宣言");
    if (!sc->var || sc->var->type->kind != TY_FUNC) {
        error_tok(tok, "関数ではありません");
    }

    Type *type = sc->var->type;
    Type *param_type = type->params;

    Node *node = new_node(ND_CALL, tok);
    node->type = type->return_type;
    node->funcname = strndup(tok->loc, tok->len);
    node->functype = type;

    if (!consume(")")) {
        Node head = {};
        Node *cur = &head;

        do {
            Node *arg = assign();
            set_node_type(arg);

            if (param_type) {
                if (param_type->kind == TY_STRUCT ||
                    param_type->kind == TY_UNION) {
                    error_tok(arg->tok,
                              "構造体または共用体の引数渡しは未対応です");
                }
                arg = new_node_cast(arg->tok, param_type, arg);
                param_type = param_type->next;
            }

            cur = cur->next = arg;
        } while (consume(","));
        expect(")");

        node->args = head.next;
    }

    return node;
}

static Node *string_literal(Token *tok) {
    Object *str_obj = new_string_literal(tok->str);
    str_obj->init_data = tok->str;
    add_global(str_obj);
    Node *node = new_node(ND_VAR, tok);
    node->var = str_obj;
    return node;
}

// primary = num | ident | callfunc
//         | string | "(" expr ")"
//         | "(" "{" stmt+ "}" ")"
//         | "sizeof" (unary | "(" typename ")")
static Node *primary() {
    Token *tok = getok();

    if (consume("(")) {
        Node *node;
        if (consume("{")) {
            node = new_node(ND_STMT_EXPR, tok->next);
            node->body = compound_stmt()->body;
        } else {
            node = expr();
        }
        expect(")");
        return node;
    }

    if (consume("sizeof")) {
        if (consume("(") && is_decl()) {
            Type *type = typename();
            expect(")");
            return new_node_num(tok, type->size);
        }
        seek(tok->next);

        Node *node = unary();
        set_node_type(node);
        return new_node_num(tok, node->type->size);
    }

    if (tok->kind == TK_IDENT) {
        seek(tok->next);
        if (consume("(")) {
            return callfunc(tok);
        }

        VarScope *sc = find_var(tok);
        if (!sc || (!sc->var && !sc->enum_type)) {
            error_tok(tok, "宣言されていない変数です");
        }

        if (sc->enum_type) {
            return new_node_num(tok, sc->enum_val);
        }

        return new_node_var(tok, sc->var);
    }

    if (tok->kind == TK_STR) {
        seek(tok->next);
        return string_literal(tok);
    }

    return new_node_num(tok, expect_number());
}
