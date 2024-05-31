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
    bool is_extern;
    int align;
};

typedef struct Initializer Initializer;
struct Initializer {
    Initializer *next;
    Type *type;
    Token *tok;
    bool is_flexible;
    Node *expr;              // 配列や構造体以外の型の場合
    Initializer **children;  // 配列や構造体の場合
};

typedef struct InitDesign InitDesign;
struct InitDesign {
    InitDesign *next;
    int index;
    Member *member;
    Object *var;
};

static Object *locals;
static Object *globals;
static Object *current_func;

static Node *gotos;
static Node *labels;

static char *break_label;
static char *continue_label;

static Node *current_switch;

static Scope *scope = &(Scope){};

static Type *declspec(VarAttr *attr);
static Type *declarator(Type *type);
static Type *typename(void);
static Type *struct_union_decl(TypeKind kind);
static Type *enum_specifier(void);
static void declaration_global(Type *base_type, VarAttr *attr);
static void parse_initializer(Initializer *init);
static Node *lvar_initializer(Object *var);
static void gvar_initializer(Object *var);
static void function(Type *type, VarAttr *attr);
static Type *params(bool *is_variadic);
static void parse_typedef(Type *base_type);
static Node *stmt(void);
static Node *compound_stmt(void);
static Node *expr_stmt(void);
static Node *expr(void);
static int64_t eval(Node *node);
static int64_t eval2(Node *node, char **label);
static int64_t eval_rval(Node *node, char **label);
static int64_t const_expr(void);
static Node *assign(void);
static Node *conditional(void);
static Node *logor(void);
static Node *logand(void);
static Node *bit_or(void);
static Node *bitxor(void);
static Node *bitand(void);
static Node *equality(void);
static Node *shift(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *cast(void);
static Node *unary(void);
static Node *postfix(void);
static Node *primary(void);

static char *tokstr(Token *tok) { return strndup(tok->loc, tok->len); }

static void enter_scope(void) {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->next = scope;
    scope = sc;
}

static void leave_scope(void) { scope = scope->next; }

static VarScope *push_scope(char *name) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->next = scope->vars;
    scope->vars = sc;
    return sc;
}

static void push_tag_scope(Token *tok, Type *type) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->name = tokstr(tok);
    sc->type = type;
    sc->next = scope->tags;
    scope->tags = sc;
}

static char *unique_name(void) {
    static int index = 0;
    char *name = calloc(20, sizeof(char));
    sprintf(name, ".LC%d", index++);
    return name;
}

static Object *new_var(Type *type, char *name) {
    Object *var = calloc(1, sizeof(Object));
    var->type = type;
    var->name = name;
    var->align = type->align;
    push_scope(name)->var = var;
    return var;
}

static Object *new_gvar(Type *type, char *name) {
    Object *var = new_var(type, name);
    var->is_static = true;
    var->is_def = true;
    var->next = globals;
    globals = var;
    return var;
}

static Object *new_anon_gvar(Type *type) {
    return new_gvar(type, unique_name());
}

static Object *new_string_literal(char *str, int len) {
    Type *type = type_array(type_char, len);
    Object *var = new_anon_gvar(type);
    var->init_data = str;
    return var;
}

static Object *new_lvar(Type *type, char *name) {
    Object *var = new_var(type, name);
    var->is_local = true;
    var->next = locals;
    locals = var;
    return var;
}

static Initializer *new_initializer(Type *type, bool is_flexible) {
    Initializer *init = calloc(1, sizeof(Initializer));
    init->type = type;

    if (type->kind == TY_ARRAY) {
        if (is_flexible && type->size < 0) {
            init->is_flexible = true;
            return init;
        }

        init->children = calloc(type->array_size, sizeof(Initializer *));
        for (int i = 0; i < type->array_size; i++) {
            init->children[i] = new_initializer(type->ptr_to, false);
        }
    } else if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
        int len = 0;
        for (Member *mem = type->members; mem; mem = mem->next) {
            len++;
        }

        init->children = calloc(len, sizeof(Initializer *));
        for (Member *mem = type->members; mem; mem = mem->next) {
            if (is_flexible && type->is_flexible && !mem->next) {
                Initializer *child = calloc(1, sizeof(Initializer));
                child->type = mem->type;
                child->is_flexible = true;
                init->children[mem->index] = child;
            } else {
                init->children[mem->index] = new_initializer(mem->type, false);
            }
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

static Member *find_member(Type *type, Token *tok) {
    for (Member *mem = type->members; mem; mem = mem->next) {
        if (equal(tok, mem->name)) {
            return mem;
        }
    }
    return NULL;
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

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *node_num(Token *tok, int64_t val) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *node_long(Token *tok, int64_t val) {
    Node *node = node_num(tok, val);
    node->type = type_long;
    return node;
}

static Node *node_ulong(Token *tok, int64_t val) {
    Node *node = node_num(tok, val);
    node->type = type_ulong;
    return node;
}

static Node *node_unary(NodeKind kind, Token *tok, Node *lhs) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

static Node *node_binary(NodeKind kind, Token *tok, Node *lhs, Node *rhs) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *node_add(Token *tok, Node *lhs, Node *rhs) {
    set_node_type(lhs);
    set_node_type(rhs);

    if (is_numeric(lhs->type) && is_numeric(rhs->type)) {
        return node_binary(ND_ADD, tok, lhs, rhs);
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

    // lhs: pointer, rhs: integer
    rhs =
        node_binary(ND_MUL, tok, rhs, node_long(tok, lhs->type->ptr_to->size));
    return node_binary(ND_ADD, tok, lhs, rhs);
}

static Node *node_sub(Token *tok, Node *lhs, Node *rhs) {
    set_node_type(lhs);
    set_node_type(rhs);

    if (is_numeric(lhs->type) && is_numeric(rhs->type)) {
        return node_binary(ND_SUB, tok, lhs, rhs);
    }

    if (is_pointer(lhs->type) && is_integer(rhs->type)) {
        rhs = node_binary(ND_MUL, tok, rhs,
                          node_long(tok, lhs->type->ptr_to->size));
        set_node_type(rhs);
        Node *node = node_binary(ND_SUB, tok, lhs, rhs);
        node->type = lhs->type;
        return node;
    }

    if (is_pointer(lhs->type) && is_pointer(rhs->type)) {
        Node *node = node_binary(ND_SUB, tok, lhs, rhs);
        node->type = type_long;
        return node_binary(ND_DIV, tok, node,
                           node_num(tok, lhs->type->ptr_to->size));
    }

    // lhs: integer, rhs: pointer
    error_tok(tok, "誤ったオペランドです");
}

static Node *node_var(Token *tok, Object *var) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

Node *node_cast(Token *tok, Type *type, Node *expr) {
    set_node_type(expr);

    Node *node = new_node(ND_CAST, tok);
    node->lhs = expr;
    node->type = calloc(1, sizeof(Type));
    *node->type = *type;
    return node;
}

static bool is_function(void) {
    if (match(";")) return false;

    Token *tok = getok();
    bool is_func = declarator(&(Type){})->kind == TY_FUNC;
    seek(tok);
    return is_func;
}

// program = (typedef | declaration_global)*
Object *program(void) {
    while (!at_eof()) {
        VarAttr attr = {};
        Type *base_type = declspec(&attr);

        if (attr.is_typedef) {
            parse_typedef(base_type);
            continue;
        }
        if (is_function()) {
            function(base_type, &attr);
            continue;
        }
        declaration_global(base_type, &attr);
    }

    return globals;
}

static bool is_decl(Token *tok) {
    static char *keywords[] = {
        "void",       "_Bool",        "char",      "short",    "int",
        "long",       "struct",       "union",     "typedef",  "enum",
        "static",     "extern",       "_Alignas",  "signed",   "unsigned",
        "const",      "volatile",     "auto",      "register", "restrict",
        "__restrict", "__restrict__", "_Noreturn", "float",    "double"};
    static int len = sizeof(keywords) / sizeof(*keywords);
    for (int i = 0; i < len; i++) {
        if (equal(tok, keywords[i])) {
            return true;
        }
    }
    return find_typedef(tok);
}

// declspec = ("void" | "_Bool"
//             | "char" | "short" | "int" | "long" | "float" | "double"
//             | "struct" struct-decl | "union" union-decl
//             | "typedef" | typedef-name | "enum" enum-specifier
//             | "static" | "extern" | "signed" | "unsigned"
//
//             | "const" | "volatile" | "auto" | "register"
//             | "restrict" | "__restrict" | "__restrict__" | "_Noreturn")+
static Type *declspec(VarAttr *attr) {
    enum {
        // clang-format off
        VOID     = 1 << 0,
        BOOL     = 1 << 2,
        CHAR     = 1 << 4,
        SHORT    = 1 << 6,
        INT      = 1 << 8,
        LONG     = 1 << 10,
        FLOAT    = 1 << 12,
        DOUBLE   = 1 << 14,
        OTHER    = 1 << 16,
        SIGNED   = 1 << 17,
        UNSIGNED = 1 << 18,
        // clang-format on
    };

    Type *type = type_int;
    int counter = 0;

    while (is_decl(getok())) {
        if (match("typedef") || match("static") || match("extern")) {
            Token *tok = getok();
            if (!attr) {
                error_tok(tok, "記憶クラス指定子は使用できません");
            }

            if (consume("typedef")) {
                attr->is_typedef = true;
            } else if (consume("static")) {
                attr->is_static = true;
            } else if (consume("extern")) {
                attr->is_extern = true;
            } else {
                unreachable();
            }

            if (attr->is_typedef && attr->is_static + attr->is_extern > 1) {
                error_tok(tok,
                          "typedef と static または extern は併用できません");
            }
            continue;
        }

        if (consume("const") || consume("volatile") || consume("auto") ||
            consume("register") || consume("restrict") ||
            consume("__restrict") || consume("__restrict__") ||
            consume("_Noreturn"))
            continue;

        if (consume("_Alignas")) {
            if (!attr) {
                error_tok(getok()->prev, "_Alignas は使えません");
            }
            expect("(");
            if (is_decl(getok())) {
                attr->align = typename()->align;
            } else {
                attr->align = const_expr();
            }
            expect(")");
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
        } else if (consume("float")) {
            counter += FLOAT;
        } else if (consume("double")) {
            counter += DOUBLE;
        } else if (consume("signed")) {
            counter |= SIGNED;
        } else if (consume("unsigned")) {
            counter |= UNSIGNED;
        } else {
            unreachable();
        }

        switch (counter) {
            case VOID:
                type = type_void;
                break;
            case BOOL:
                type = type_bool;
                break;
            case CHAR:
            case SIGNED + CHAR:
                type = type_char;
                break;
            case UNSIGNED + CHAR:
                type = type_uchar;
                break;
            case SHORT:
            case SHORT + INT:
            case SIGNED + SHORT:
            case SIGNED + SHORT + INT:
                type = type_short;
                break;
            case UNSIGNED + SHORT:
            case UNSIGNED + SHORT + INT:
                type = type_ushort;
                break;
            case INT:
            case SIGNED:
            case SIGNED + INT:
                type = type_int;
                break;
            case UNSIGNED:
            case UNSIGNED + INT:
                type = type_uint;
                break;
            case LONG:
            case LONG + INT:
            case LONG + LONG:
            case LONG + LONG + INT:
            case SIGNED + LONG:
            case SIGNED + LONG + INT:
            case SIGNED + LONG + LONG:
            case SIGNED + LONG + LONG + INT:
                type = type_long;
                break;
            case UNSIGNED + LONG:
            case UNSIGNED + LONG + INT:
            case UNSIGNED + LONG + LONG:
            case UNSIGNED + LONG + LONG + INT:
                type = type_ulong;
                break;
            case FLOAT:
                type = type_float;
                break;
            case DOUBLE:
                type = type_double;
                break;
            default:
                error_tok(getok()->prev, "不正な型です");
        }
    }

    return type;
}

// declsuffix = "(" params ")" |  "[" const_expr? "]" declsuffix
static Type *declsuffix(Type *type) {
    if (consume("(")) {
        bool is_variadic = false;
        type = type_func(type, params(&is_variadic));
        type->is_variadic = is_variadic;
        return type;
    }

    if (consume("[")) {
        while (consume("static") || consume("restrict"));

        int len = match("]") ? -1 : const_expr();
        expect("]");
        type = declsuffix(type);
        return type_array(type, len);
    }

    return type;
}

// pointers = ("*" ("const" | "volatile" | "restrict")*)*
static Type *pointers(Type *type) {
    while (consume("*")) {
        type = type_ptr(type);
        while (consume("const") || consume("volatile") || consume("restrict") ||
               consume("__restrict") || consume("__restrict__"));
    }

    return type;
}

// declarator = pointers ("(" ident ")" | "(" declarator ")" | ident) declsuffix
static Type *declarator(Type *type) {
    type = pointers(type);

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

    Token *name = getok();
    if (name->kind == TK_IDENT) {
        seek(name->next);
    }

    type = declsuffix(type);
    type->name = name;

    return type;
}

// abstract-declarator = pointers ("(" abstract-declarator ")")? declsuffix
static Type *abstract_declarator(Type *type) {
    type = pointers(type);

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

static Type *typename(void) { return abstract_declarator(declspec(NULL)); }

// declaration = declspec
//               (declarator ("=" assign)? ("," declarator ("=" assign)?)*)? ";"
static Node *declaration_local(Type *base_type, VarAttr *attr) {
    Node *node = new_node(ND_BLOCK, getok());

    if (!consume(";")) {
        Node head = {};
        Node *cur = &head;

        do {
            Type *type = declarator(base_type);
            if (type->kind == TY_VOID) {
                error_tok(type->name, "void 型の変数が宣言されました");
            }
            if (type->name->kind != TK_IDENT) {
                error_tok(type->name, "変数名がありません");
            }

            if (attr && attr->is_static) {
                Object *var = new_anon_gvar(type);
                char *name = tokstr(type->name);
                push_scope(name)->var = var;
                if (consume("=")) gvar_initializer(var);
                continue;
            }

            check_var_redef(type->name);

            Object *var = new_lvar(type, tokstr(type->name));
            if (attr && attr->align) {
                var->align = attr->align;
            }

            if (consume("=")) {
                Token *tok = getok();
                Node *expr = lvar_initializer(var);
                cur = cur->next = node_unary(ND_EXPR_STMT, tok, expr);
            }

            if (type->kind == TY_VOID) {
                error_tok(type->name, "void 型の変数が宣言されました");
            }
            if (var->type->size < 0) {
                error_tok(type->name, "不完全な型です");
            }
        } while (consume(","));
        expect(";");

        node->body = head.next;
    }

    return node;
}

static void declaration_global(Type *base_type, VarAttr *attr) {
    if (consume(";")) return;

    do {
        Type *type = declarator(base_type);
        if (type->kind == TY_VOID) {
            error_tok(type->name, "void 型の変数が宣言されました");
        }
        if (type->name->kind != TK_IDENT) {
            error_tok(type->name, "変数名がありません");
        }

        // グローバル変数の再宣言は可能
        check_var_redef(type->name);

        Object *var = new_gvar(type, tokstr(type->name));
        var->is_def = !attr->is_extern;
        var->is_static = attr->is_static;
        var->align = attr->align ? attr->align : var->align;

        if (consume("=")) {
            gvar_initializer(var);
        }

    } while (consume(","));
    expect(";");
}

static bool is_end(void) {
    return match("}") || (match(",") && equal(getok()->next, "}"));
}

static bool consume_end(void) {
    Token *tok = getok();
    if (match("}")) {
        seek(tok->next);
        return true;
    }

    if (match(",") && equal(tok->next, "}")) {
        seek(tok->next->next);
        return true;
    }

    return false;
}

static void skip_excess_element(void) {
    if (consume("{")) {
        do {
            skip_excess_element();
        } while (!consume_end());
        return;
    }
    assign();
}

// string-initializer = string
static void string_initalizer(Initializer *init) {
    Token *tok = getok();

    if (init->is_flexible) {
        *init = *new_initializer(
            type_array(init->type->ptr_to, tok->type->array_size), false);
    }

    int len = MIN(init->type->array_size, tok->type->array_size);
    for (int i = 0; i < len; i++) {
        init->children[i]->expr = node_num(tok, tok->str[i]);
    }
    seek(tok->next);
}

static int count_array_init_elements(Type *type) {
    Initializer *dummy = new_initializer(type->ptr_to, false);
    Token *tok = getok();
    int i;
    for (i = 0; !is_end(); i++) {
        if (i > 0) expect(",");
        parse_initializer(dummy);
    }
    seek(tok);
    return i;
}

// array-initializer1 = initializer ("," initializer)* ","? "}"
static void array_initializer1(Initializer *init) {
    if (init->is_flexible) {
        int len = count_array_init_elements(init->type);
        *init = *new_initializer(type_array(init->type->ptr_to, len), false);
    }

    for (int i = 0; !consume_end(); i++) {
        if (i > 0) expect(",");

        if (i < init->type->array_size) {
            parse_initializer(init->children[i]);
        } else {
            skip_excess_element();
        }
    }
}

// array-initializer2 = initializer ("," initializer)*
static void array_initializer2(Initializer *init) {
    if (init->is_flexible) {
        int len = count_array_init_elements(init->type);
        *init = *new_initializer(type_array(init->type->ptr_to, len), false);
    }

    for (int i = 0; i < init->type->array_size && !is_end(); i++) {
        if (i > 0) expect(",");
        parse_initializer(init->children[i]);
    }
}

// struct-initializer1 = initializer ("," initializer)* ","? "}"
static void struct_initializer1(Initializer *init) {
    for (Member *mem = init->type->members; !consume_end(); mem = mem->next) {
        if (mem != init->type->members) expect(",");
        if (!mem) {
            skip_excess_element();
            continue;
        }
        parse_initializer(init->children[mem->index]);
    }
}
// struct-initializer2 = initializer ("," initializer)*
static void struct_initializer2(Initializer *init) {
    Token *tok = getok();
    Node *expr = assign();
    set_node_type(expr);

    if (expr->type->kind == TY_STRUCT) {
        init->expr = expr;
        return;
    }

    seek(tok);
    for (Member *mem = init->type->members; mem && !is_end(); mem = mem->next) {
        if (mem != init->type->members) expect(",");
        parse_initializer(init->children[mem->index]);
    }
}
// union-initializer = "{" initializer ("," initializer)* "}"
static void union_initializer(Initializer *init) {
    if (consume("{")) {
        parse_initializer(init->children[0]);
        consume(",");
        expect("}");
        return;
    }

    Token *tok = getok();
    Node *expr = assign();

    set_node_type(expr);
    if (expr->type->kind == TY_UNION) {
        init->expr = expr;
        return;
    }

    seek(tok);
    parse_initializer(init->children[0]);
}

static void parse_initializer(Initializer *init) {
    if (init->type->kind == TY_ARRAY) {
        if (getok()->kind == TK_STR) {
            string_initalizer(init);
            return;
        }
        if (consume("{")) {
            array_initializer1(init);
        } else {
            array_initializer2(init);
        }
        return;
    }

    if (init->type->kind == TY_STRUCT) {
        if (consume("{")) {
            struct_initializer1(init);
        } else {
            struct_initializer2(init);
        }
        return;
    }

    if (init->type->kind == TY_UNION) {
        union_initializer(init);
        return;
    }

    if (consume("{")) {
        init->expr = assign();
        expect("}");
        return;
    }

    init->expr = assign();
}

static Type *copy_struct_type(Type *type) {
    type = copy_type(type);

    Member head = {};
    Member *cur = &head;
    for (Member *mem = type->members; mem; mem = mem->next) {
        Member *m = calloc(1, sizeof(Member));
        *m = *mem;
        cur = cur->next = m;
    }

    type->members = head.next;
    return type;
}

// initializer = string_initalizer | array-initializer
//             | struct-initializer | union-initializer
//             | assign
static Initializer *initializer(Type *type, Type **new_type) {
    Initializer *init = new_initializer(type, true);
    parse_initializer(init);

    if ((type->kind == TY_STRUCT || type->kind == TY_UNION) &&
        type->is_flexible) {
        type = copy_struct_type(type);

        Member *mem = type->members;
        while (mem->next) mem = mem->next;
        mem->type = init->children[mem->index]->type;
        type->size += mem->type->size;

        *new_type = type;
    } else {
        *new_type = init->type;
    }

    return init;
}

static Node *init_design_expr(InitDesign *design) {
    Token *tok = getok();
    if (design->var) {
        return node_var(tok, design->var);
    }
    if (design->member) {
        Node *node = node_unary(ND_MEMBER, tok, init_design_expr(design->next));
        node->member = design->member;
        return node;
    }
    Node *lhs = init_design_expr(design->next);
    Node *rhs = node_num(tok, design->index);
    return node_unary(ND_DEREF, tok, node_add(tok, lhs, rhs));
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
            node = node_binary(ND_COMMA, tok, node, rhs);
        }

        return node;
    }

    if (type->kind == TY_STRUCT && !init->expr) {
        Node *node = new_node(ND_NULL_EXPR, tok);
        for (Member *mem = type->members; mem; mem = mem->next) {
            InitDesign design2 = {design, 0, mem};
            Node *rhs = create_lvar_init(init->children[mem->index], mem->type,
                                         &design2);
            node = node_binary(ND_COMMA, tok, node, rhs);
        }
        return node;
    }

    if (type->kind == TY_UNION && !init->expr) {
        InitDesign design2 = {design, 0, type->members};
        return create_lvar_init(init->children[0], type->members->type,
                                &design2);
    }

    if (!init->expr) {
        return new_node(ND_NULL_EXPR, tok);
    }

    Node *lhs = init_design_expr(design);
    Node *rhs = init->expr;
    return node_binary(ND_ASSIGN, tok, lhs, rhs);
}

static Node *lvar_initializer(Object *var) {
    Initializer *init = initializer(var->type, &var->type);
    InitDesign design = {NULL, 0, NULL, var};
    Token *tok = getok();

    Node *lhs = new_node(ND_MEMZERO, tok);
    lhs->var = var;

    Node *rhs = create_lvar_init(init, var->type, &design);
    return node_binary(ND_COMMA, tok, lhs, rhs);
}

static void write_buf(char *buf, uint64_t val, int size) {
    switch (size) {
        case 1:
            *buf = val;
            return;
        case 2:
            *(uint16_t *)buf = val;
            return;
        case 4:
            *(uint32_t *)buf = val;
            return;
        case 8:
            *(uint64_t *)buf = val;
            return;
        default:
            unreachable();
    }
}

static Relocation *write_gvar_data(Relocation *cur, Initializer *init,
                                   Type *type, char *buf, int offset) {
    if (type->kind == TY_ARRAY) {
        int size = type->ptr_to->size;
        for (int i = 0; i < type->array_size; i++) {
            cur = write_gvar_data(cur, init->children[i], type->ptr_to, buf,
                                  offset + size * i);
        }
        return cur;
    }

    if (type->kind == TY_STRUCT) {
        for (Member *mem = type->members; mem; mem = mem->next) {
            cur = write_gvar_data(cur, init->children[mem->index], mem->type,
                                  buf, offset + mem->offset);
        }
        return cur;
    }

    if (type->kind == TY_UNION) {
        return write_gvar_data(cur, init->children[0], type->members->type, buf,
                               offset);
    }

    if (!init->expr) {
        return cur;
    }

    char *label = NULL;
    uint64_t val = eval2(init->expr, &label);

    if (!label) {
        write_buf(buf + offset, val, type->size);
        return cur;
    }

    Relocation *rel = calloc(1, sizeof(Relocation));
    rel->offset = offset;
    rel->label = label;
    rel->addend = val;
    cur->next = rel;
    return cur->next;
}

static void gvar_initializer(Object *var) {
    Initializer *init = initializer(var->type, &var->type);

    Relocation head = {};
    char *buf = calloc(1, var->type->size);
    write_gvar_data(&head, init, var->type, buf, 0);
    var->init_data = buf;
    var->rel = head.next;
}

static void add_params_lvar(Type *param) {
    if (param) {
        if (param->name->kind != TK_IDENT) {
            error_tok(param->name, "引数名がありません");
        }
        add_params_lvar(param->next);
        new_lvar(param, tokstr(param->name));
    }
}

static void resolve_goto_labels(void) {
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
    if (type->name->kind != TK_IDENT) {
        error_tok(type->name, "関数名がありません");
    }
    Token *tok = type->name;

    bool is_def = !consume(";");
    if (is_def) {
        check_var_redef(tok);
        check_func_redef(tok);
    }
    Object *func = new_gvar(type, tokstr(tok));
    func->is_func = true;
    func->is_def = is_def;
    func->is_static = attr->is_static;

    if (!func->is_def) {
        return;
    }

    locals = NULL;
    current_func = func;

    enter_scope();

    add_params_lvar(type->params);
    func->params = locals;
    if (type->is_variadic) {
        func->va_area = new_lvar(type_array(type_char, 136), "__va_area__");
    }

    expect("{");
    func->body = compound_stmt();

    func->locals = locals;

    leave_scope();
    resolve_goto_labels();
}

// params = ("void" | param ("," param)* ("," "...")?)? ")"
static Type *params(bool *is_variadic) {
    if (consume(")")) {
        *is_variadic = true;
        return NULL;
    }
    if (match("void") && equal(getok()->next, ")")) {
        seek(getok()->next->next);
        return NULL;
    }

    Type head = {};
    Type *cur = &head;

    do {
        if (consume("...")) {
            *is_variadic = true;
            break;
        }
        Type *type = copy_type(declarator(declspec(NULL)));
        if (type->kind == TY_ARRAY) {
            Token *name = type->name;
            type = type_ptr(type->ptr_to);
            type->name = name;
        }
        cur = cur->next = type;
    } while (consume(","));
    expect(")");

    return head.next;
}

// members = (declspec declarator ("," declarator)* ";")*
static Member *members(bool *is_flexible) {
    Member head = {};
    Member *cur = &head;
    int index = 0;

    while (!consume("}")) {
        if (consume(";")) continue;

        VarAttr attr = {};
        Type *base_type = declspec(&attr);

        do {
            Member *mem = calloc(1, sizeof(Member));
            mem->type = declarator(base_type);
            mem->name = tokstr(mem->type->name);
            mem->index = index++;
            mem->align = attr.align ? attr.align : mem->type->align;
            cur = cur->next = mem;
        } while (consume(","));
        expect(";");
    }

    if (cur != &head && cur->type->kind == TY_ARRAY &&
        cur->type->array_size < 0) {
        cur->type = type_array(cur->type->ptr_to, 0);
        *is_flexible = true;
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
                error_tok(tag, "%sのタグではありません",
                          kind == TY_STRUCT ? "構造体" : "共用体");
            }
            return type;
        }
        type = kind == TY_STRUCT ? type_struct(NULL) : type_union(NULL);
        type->size = -1;
        push_tag_scope(tag, type);
        return type;
    }

    expect("{");

    bool is_flexible = false;
    Member *mem = members(&is_flexible);
    Type *type = kind == TY_STRUCT ? type_struct(mem) : type_union(mem);
    type->is_flexible = is_flexible;

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
// enum-list      = ident ("=" const_expr)? ("," ident ("=" const_expr)?)* ","?
static Type *enum_specifier(void) {
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

    Type *type = type_enum();
    if (!consume_end()) {
        int val = 0;
        do {
            Token *tok = expect_ident();
            char *name = tokstr(tok);

            if (consume("=")) {
                val = const_expr();
            }

            VarScope *sc = push_scope(name);
            sc->enum_type = type;
            sc->enum_val = val++;
        } while (!consume_end() && consume(","));
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

    Node *node = node_unary(ND_MEMBER, tok, lhs);
    node->member = find_member(lhs->type, tok);
    if (!node->member) {
        error_tok(tok, "存在しないメンバです");
    }

    return node;
}

static void parse_typedef(Type *base_type) {
    if (consume(";")) return;

    do {
        Type *type = declarator(base_type);
        if (type->name->kind != TK_IDENT) {
            error_tok(type->name, "型名がありません");
        }
        char *name = tokstr(type->name);
        check_var_redef(type->name);
        push_scope(name)->type_def = type;
    } while (consume(","));
    expect(";");
}

// stmt = expr_stmt
//      | "{" compound_stmt
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "do" stmt "while" "(" expr ")" ";"
//      | "goto" ident ";"
//      | ident ":" stmt
//      | "break" ";"
//      | "continue" ";"
//      | "switch" "(" expr ")" stmt
//      | (case const_expr | "default") ":" stmt
//      | "return" expr? ";"
static Node *stmt(void) {
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

        if (is_decl(getok())) {
            node->init = declaration_local(declspec(NULL), NULL);
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

    if (consume("do")) {
        Node *node = new_node(ND_DO, tok);

        char *brk = break_label;
        char *cont = continue_label;
        break_label = node->break_label = unique_name();
        continue_label = node->continue_label = unique_name();

        node->then = stmt();

        break_label = brk;
        continue_label = cont;

        expect("while");
        expect("(");
        node->cond = expr();
        expect(")");
        expect(";");

        return node;
    }

    if (consume("goto")) {
        Node *node = new_node(ND_GOTO, tok);
        Token *label = expect_ident();
        expect(";");

        node->label = tokstr(label);
        node->goto_next = gotos;
        gotos = node;

        return node;
    }

    if (tok->kind == TK_IDENT && equal(tok->next, ":")) {
        seek(tok->next->next);
        Node *node = node_unary(ND_LABEL, tok, stmt());

        node->label = tokstr(tok);
        node->unique_label = unique_name();
        node->goto_next = labels;
        labels = node;

        return node;
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

        Node *node = node_unary(ND_CASE, tok, stmt());
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

        Node *node = node_unary(ND_CASE, tok, stmt());
        node->label = unique_name();
        current_switch->default_case = node;
        return node;
    }

    if (consume("return")) {
        if (consume(";")) return new_node(ND_RETURN, tok);

        Node *node = expr();
        expect(";");
        set_node_type(node);
        node = node_cast(node->tok, current_func->type->return_type, node);
        node = node_unary(ND_RETURN, tok, node);
        return node;
    }

    return expr_stmt();
}

// compound_stmt = (typedef | declaration_local | stmt)* "}"
static Node *compound_stmt(void) {
    Node *node = new_node(ND_BLOCK, getok()->prev);

    Node head = {};
    Node *cur = &head;

    enter_scope();

    while (!consume("}")) {
        if (is_decl(getok()) && !equal(getok()->next, ":")) {
            VarAttr attr = {};
            Type *base_type = declspec(&attr);

            if (attr.is_typedef) {
                parse_typedef(base_type);
                continue;
            }

            if (is_function()) {
                function(base_type, &attr);
                continue;
            }

            if (attr.is_extern) {
                declaration_global(base_type, &attr);
                continue;
            }

            cur->next = declaration_local(base_type, &attr);
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
static Node *expr_stmt(void) {
    Token *tok = getok();
    if (consume(";")) {
        return new_node(ND_BLOCK, tok);
    }

    Node *node = node_unary(ND_EXPR_STMT, tok, expr());
    expect(";");
    return node;
}

// expr = assign ("," expr)?
static Node *expr(void) {
    Node *node = assign();
    if (consume(",")) {
        node = node_binary(ND_COMMA, getok()->prev, node, expr());
    }
    return node;
}

static int64_t eval(Node *node) { return eval2(node, NULL); }

static int64_t eval2(Node *node, char **label) {
    set_node_type(node);
    switch (node->kind) {
        case ND_ADD:
            return eval2(node->lhs, label) + eval(node->rhs);
        case ND_SUB:
            return eval2(node->lhs, label) - eval(node->rhs);
        case ND_MUL:
            return eval(node->lhs) * eval(node->rhs);
        case ND_DIV:
            if (node->type->is_unsigned) {
                return (uint64_t)eval(node->lhs) / (uint64_t)eval(node->rhs);
            }
            return eval(node->lhs) / eval(node->rhs);
        case ND_MOD:
            if (node->type->is_unsigned) {
                return (uint64_t)eval(node->lhs) % eval(node->rhs);
            }
            return eval(node->lhs) % eval(node->rhs);
        case ND_EQ:
            return eval(node->lhs) == eval(node->rhs);
        case ND_NEQ:
            return eval(node->lhs) != eval(node->rhs);
        case ND_LS:
            if (node->lhs->type->is_unsigned) {
                return (uint64_t)eval(node->lhs) < eval(node->rhs);
            }
            return eval(node->lhs) < eval(node->rhs);
        case ND_LEQ:
            if (node->lhs->type->is_unsigned) {
                return (uint64_t)eval(node->lhs) <= eval(node->rhs);
            }
            return eval(node->lhs) <= eval(node->rhs);
        case ND_AND:
            return eval(node->lhs) && eval(node->rhs);
        case ND_OR:
            return eval(node->lhs) || eval(node->rhs);
        case ND_BITAND:
            return eval(node->lhs) & eval(node->rhs);
        case ND_BITOR:
            return eval(node->lhs) | eval(node->rhs);
        case ND_BITXOR:
            return eval(node->lhs) ^ eval(node->rhs);
        case ND_BITSHL:
            return eval(node->lhs) << eval(node->rhs);
        case ND_BITSHR:
            if (node->type->is_unsigned && node->type->size == 8) {
                return (uint64_t)eval(node->lhs) >> eval(node->lhs);
            }
            return eval(node->lhs) >> eval(node->rhs);
        case ND_COMMA:
            return eval2(node->rhs, label);
        case ND_COND:
            return eval(node->cond) ? eval2(node->then, label)
                                    : eval2(node->els, NULL);
        case ND_NEG:
            return -eval(node->lhs);
        case ND_NOT:
            return !eval(node->lhs);
        case ND_BITNOT:
            return ~eval(node->lhs);
        case ND_CAST: {
            int64_t val = eval2(node->lhs, label);
            if (is_integer(node->type)) {
                switch (node->type->size) {
                    case 1:
                        return node->type->is_unsigned ? (uint8_t)val
                                                       : (int8_t)val;
                    case 2:
                        return node->type->is_unsigned ? (uint16_t)val
                                                       : (int16_t)val;
                    case 4:
                        return node->type->is_unsigned ? (uint32_t)val
                                                       : (int32_t)val;
                }
            }
            return val;
        }
        case ND_ADDR:
            return eval_rval(node->lhs, label);
        case ND_MEMBER:
            if (!label) {
                error_tok(node->tok, "コンパイル時に定数ではありません");
            }
            if (node->type->kind != TY_ARRAY) {
                error_tok(node->tok, "誤った初期化子です");
            }
            return eval_rval(node->lhs, label) + node->member->offset;
        case ND_VAR:
            if (!label) {
                error_tok(node->tok, "コンパイル時に定数ではありません");
            }
            if (node->var->type->kind != TY_ARRAY &&
                node->var->type->kind != TY_FUNC) {
                error_tok(node->tok, "誤った初期化子です");
            }
            *label = node->var->name;
            return 0;
        case ND_NUM:
            return node->val;
    }

    error_tok(node->tok, "コンパイル時に定数ではありません");
}

static int64_t eval_rval(Node *node, char **label) {
    switch (node->kind) {
        case ND_VAR:
            if (node->var->is_local) {
                error_tok(node->tok, "コンパイル時に定数ではありません");
            }
            *label = node->var->name;
            return 0;
        case ND_DEREF:
            return eval2(node->lhs, label);
        case ND_MEMBER:
            return eval_rval(node->lhs, label) + node->member->offset;
    }

    error_tok(node->tok, "誤った初期化子です");
}

static int64_t const_expr(void) { return eval(conditional()); }

static Node *to_assign(Node *binary) {
    set_node_type(binary->lhs);
    set_node_type(binary->rhs);

    Token *tok = binary->tok;
    Object *var = new_lvar(type_ptr(binary->lhs->type), "");

    Node *expr1, *expr2;

    expr1 = node_unary(ND_ADDR, tok, binary->lhs);
    expr1 = node_binary(ND_ASSIGN, tok, node_var(tok, var), expr1);

    expr2 = node_unary(ND_DEREF, tok, node_var(tok, var));
    expr2 = node_binary(binary->kind, tok, expr2, binary->rhs);
    expr2 = node_binary(ND_ASSIGN, tok,
                        node_unary(ND_DEREF, tok, node_var(tok, var)), expr2);

    return node_binary(ND_COMMA, tok, expr1, expr2);
}

// assign = conditional (
//            ("=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" |
//            "<<=" | ">>=") assign
//          )?
static Node *assign(void) {
    Token *tok = getok();

    Node *node = conditional();
    if (consume("=")) {
        node = node_binary(ND_ASSIGN, tok, node, assign());
    }
    if (consume("+=")) {
        node = to_assign(node_add(tok, node, assign()));
    }
    if (consume("-=")) {
        node = to_assign(node_sub(tok, node, assign()));
    }
    if (consume("*=")) {
        node = to_assign(node_binary(ND_MUL, tok, node, assign()));
    }
    if (consume("/=")) {
        node = to_assign(node_binary(ND_DIV, tok, node, assign()));
    }
    if (consume("%=")) {
        node = to_assign(node_binary(ND_MOD, tok, node, assign()));
    }
    if (consume("&=")) {
        node = to_assign(node_binary(ND_BITAND, tok, node, assign()));
    }
    if (consume("|=")) {
        node = to_assign(node_binary(ND_BITOR, tok, node, assign()));
    }
    if (consume("^=")) {
        node = to_assign(node_binary(ND_BITXOR, tok, node, assign()));
    }
    if (consume("<<=")) {
        node = to_assign(node_binary(ND_BITSHL, tok, node, assign()));
    }
    if (consume(">>=")) {
        node = to_assign(node_binary(ND_BITSHR, tok, node, assign()));
    }
    return node;
}

// conditional = logor ("?" expr ":" conditional)?
static Node *conditional(void) {
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
static Node *logor(void) {
    Node *node = logand();
    while (consume("||")) {
        node = node_binary(ND_OR, getok()->prev, node, logand());
    }
    return node;
}

// logand = bitor ("&&" bitor)*
static Node *logand(void) {
    Node *node = bit_or();
    while (consume("&&")) {
        node = node_binary(ND_AND, getok()->prev, node, bit_or());
    }
    return node;
}

// bitor = bitxor ("|" bitxor)*
static Node *bit_or(void) {
    Node *node = bitxor();
    while (consume("|")) {
        node = node_binary(ND_BITOR, getok()->prev, node, bitxor());
    }
    return node;
}

// bitxor = bitand ("^" bitand)*
static Node *bitxor(void) {
    Node *node = bitand();
    while (consume("^")) {
        node = node_binary(ND_BITXOR, getok()->prev, node, bitand());
    }
    return node;
}
// bitand = equality ("&" equality)*
static Node *bitand(void) {
    Node *node = equality();
    while (consume("&")) {
        node = node_binary(ND_BITAND, getok()->prev, node, equality());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
    Node *node = relational();

    while (true) {
        Token *tok = getok();
        if (consume("==")) {
            node = node_binary(ND_EQ, tok, node, relational());
        } else if (consume("!=")) {
            node = node_binary(ND_NEQ, tok, node, relational());
        } else {
            return node;
        }
    }
}

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static Node *relational(void) {
    Node *node = shift();

    while (true) {
        Token *tok = getok();
        if (consume("<=")) {
            node = node_binary(ND_LEQ, tok, node, shift());
        } else if (consume("<")) {
            node = node_binary(ND_LS, tok, node, shift());
        } else if (consume(">=")) {
            node = node_binary(ND_LEQ, tok, shift(), node);
        } else if (consume(">")) {
            node = node_binary(ND_LS, tok, shift(), node);
        } else {
            return node;
        }
    }
}

// shift = add ("<<" add | ">>" add)
static Node *shift(void) {
    Node *node = add();

    while (true) {
        Token *tok = getok();
        if (consume("<<")) {
            node = node_binary(ND_BITSHL, tok, node, add());
        } else if (consume(">>")) {
            node = node_binary(ND_BITSHR, tok, node, add());
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
    Node *node = mul();

    while (true) {
        Token *tok = getok();
        if (consume("+")) {
            node = node_add(tok, node, mul());
        } else if (consume("-")) {
            node = node_sub(tok, node, mul());
        } else {
            return node;
        }
    }
}

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node *mul(void) {
    Node *node = cast();

    while (true) {
        Token *tok = getok();
        if (consume("*")) {
            node = node_binary(ND_MUL, tok, node, cast());
        } else if (consume("/")) {
            node = node_binary(ND_DIV, tok, node, cast());
        } else if (consume("%")) {
            node = node_binary(ND_MOD, tok, node, cast());
        } else {
            return node;
        }
    }
}

// cast = "(" typename ")" cast | unary
static Node *cast(void) {
    Token *tok = getok();
    if (match("(") && is_decl(tok->next)) {
        seek(tok->next);
        Type *type = typename();
        expect(")");

        if (match("{")) {
            seek(tok);
            return unary();
        }
        return node_cast(tok, type, cast());
    }

    return unary();
}

// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
static Node *unary(void) {
    Token *tok = getok();
    if (consume("+")) {
        return cast();
    }
    if (consume("-")) {
        return node_unary(ND_NEG, tok, cast());
    }
    if (consume("*")) {
        return node_unary(ND_DEREF, tok, cast());
    }
    if (consume("&")) {
        return node_unary(ND_ADDR, tok, cast());
    }
    if (consume("!")) {
        return node_unary(ND_NOT, tok, cast());
    }
    if (consume("~")) {
        return node_unary(ND_BITNOT, tok, cast());
    }
    if (consume("++")) {
        return to_assign(node_add(tok, unary(), node_num(tok, 1)));
    }
    if (consume("--")) {
        return to_assign(node_sub(tok, unary(), node_num(tok, 1)));
    }
    return postfix();
}

// postfix = "(" typename ")" initializer
//         | primary ("[" expr "]" | ("." | "->") ident | "++" | "--")*
static Node *postfix(void) {
    if (match("(") && is_decl(getok()->next)) {
        Token *start = getok();

        seek(start->next);
        Type *type = typename();
        expect(")");

        if (scope->next) {
            Object *var = new_lvar(type, "");
            Token *tok = getok();
            Node *lhs = lvar_initializer(var);
            Node *rhs = node_var(tok, var);
            return node_binary(ND_COMMA, start, lhs, rhs);
        } else {
            Object *var = new_anon_gvar(type);
            gvar_initializer(var);
            return node_var(start, var);
        }
    }

    Node *node = primary();
    while (true) {
        Token *tok = getok();
        if (consume("[")) {
            Node *idx = expr();
            expect("]");

            node = node_unary(ND_DEREF, tok, node_add(tok, node, idx));
            continue;
        }

        if (consume(".")) {
            node = struct_union_ref(node);
            continue;
        }

        if (consume("->")) {
            node = node_unary(ND_DEREF, tok, node);
            node = struct_union_ref(node);
            continue;
        }

        if (consume("++")) {
            set_node_type(node);
            Type *type = node->type;

            node = to_assign(node_add(tok, node, node_num(tok, 1)));
            node = node_sub(tok, node, node_num(tok, 1));
            node = node_cast(tok, type, node);
            continue;
        }

        if (consume("--")) {
            set_node_type(node);
            Type *type = node->type;

            node = to_assign(node_sub(tok, node, node_num(tok, 1)));
            node = node_add(tok, node, node_num(tok, 1));
            node = node_cast(tok, type, node);
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
    node->funcname = tokstr(tok);
    node->functype = type;

    if (!consume(")")) {
        Node head = {};
        Node *cur = &head;

        do {
            Node *arg = assign();
            set_node_type(arg);

            if (!param_type && !type->is_variadic) {
                error_tok(arg->tok, "引数が多すぎます");
            }

            if (param_type) {
                if (param_type->kind == TY_STRUCT ||
                    param_type->kind == TY_UNION) {
                    error_tok(arg->tok,
                              "構造体または共用体の引数渡しは未対応です");
                }
                arg = node_cast(arg->tok, param_type, arg);
                param_type = param_type->next;
            } else if (arg->type->kind == TY_FLOAT) {
                arg = node_cast(arg->tok, type_double, arg);
            }

            cur = cur->next = arg;
        } while (consume(","));

        if (param_type) {
            error_tok(getok(), "引数が少なすぎます");
        }

        expect(")");

        node->args = head.next;
    }

    return node;
}

static Node *string_literal(Token *tok) {
    Object *str_obj = new_string_literal(tok->str, tok->type->array_size);
    Node *node = new_node(ND_VAR, tok);
    node->var = str_obj;
    return node;
}

// primary = num | ident | callfunc
//         | string | "(" expr ")"
//         | "(" "{" stmt+ "}" ")"
//         | "sizeof" (unary | "(" typename ")")
//         | "_Alignof" (unary | "(" typename ")")
static Node *primary(void) {
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
        if (match("(") && is_decl(getok()->next)) {
            seek(getok()->next);
            Type *type = typename();
            expect(")");
            return node_ulong(tok, type->size);
        }

        Node *node = unary();
        set_node_type(node);
        return node_ulong(tok, node->type->size);
    }

    if (consume("_Alignof")) {
        if (match("(") && is_decl(getok()->next)) {
            seek(getok()->next);
            Type *type = typename();
            expect(")");
            return node_ulong(tok, type->align);
        }

        Node *node = unary();
        set_node_type(node);
        return node_ulong(tok, node->type->align);
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
            return node_num(tok, sc->enum_val);
        }

        return node_var(tok, sc->var);
    }

    if (tok->kind == TK_STR) {
        seek(tok->next);
        return string_literal(tok);
    }

    if (tok->kind == TK_NUM) {
        seek(tok->next);
        Node *node = new_node(ND_NUM, tok);
        node->type = tok->type;

        if (is_floatnum(node->type)) {
            node->fval = tok->fval;
        } else {
            node->val = tok->val;
        }

        return node;
    }

    error_tok(tok, "式ではありません");
}
