#include <stdlib.h>

#include "lion.h"

// clang-format off
Type *type_void   = &(Type){TY_VOID,   1, 1};
Type *type_bool   = &(Type){TY_BOOL,   1, 1};

Type *type_char   = &(Type){TY_CHAR,   1, 1};
Type *type_short  = &(Type){TY_SHORT,  2, 2};
Type *type_int    = &(Type){TY_INT,    4, 4};
Type *type_long   = &(Type){TY_LONG,   8, 8};

Type *type_uchar  = &(Type){TY_CHAR,   1, 1, true};
Type *type_ushort = &(Type){TY_SHORT,  2, 2, true};
Type *type_uint   = &(Type){TY_INT,    4, 4, true};
Type *type_ulong  = &(Type){TY_LONG,   8, 8, true};

Type *type_float  = &(Type){TY_FLOAT,  4, 4};
Type *type_double = &(Type){TY_DOUBLE, 8, 8};
// clang-format on

static Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *type_func(Type *return_type, Type *params) {
    Type *type = new_type(TY_FUNC);
    type->return_type = return_type;
    type->params = params;
    return type;
}

Type *type_enum(void) {
    Type *type = new_type(TY_ENUM);
    type->size = type->align = 4;
    return type;
}

Type *type_ptr(Type *base_type) {
    Type *type = new_type(TY_PTR);
    type->ptr_to = base_type;

    type->size = 8;
    type->align = type->size;
    return type;
}

Type *type_array(Type *base_type, int size) {
    Type *type = new_type(TY_ARRAY);
    type->ptr_to = base_type;
    type->array_size = size;

    type->size = size * base_type->size;
    type->align = base_type->align;
    return type;
}

Type *type_struct(Member *members) {
    Type *type = new_type(TY_STRUCT);
    type->members = members;
    type->align = 1;

    int offset = 0;
    for (Member *mem = type->members; mem; mem = mem->next) {
        offset = align(offset, mem->align);
        mem->offset = offset;
        offset += mem->type->size;

        if (type->align < mem->align) {
            type->align = mem->align;
        }
    }
    type->size = align(offset, type->align);

    return type;
}

Type *type_union(Member *members) {
    Type *type = new_type(TY_UNION);
    type->members = members;
    type->align = 1;

    for (Member *mem = type->members; mem; mem = mem->next) {
        if (type->align < mem->align) {
            type->align = mem->align;
        }
        if (type->size < mem->type->size) {
            type->size = mem->type->size;
        }
    }
    type->size = align(type->size, type->align);

    return type;
}

Type *copy_type(Type *type) {
    Type *cp = calloc(1, sizeof(Type));
    *cp = *type;
    return cp;
}

bool is_pointer(Type *type) { return type->ptr_to != NULL; }

bool is_integer(Type *type) {
    return type->kind == TY_BOOL || type->kind == TY_CHAR ||
           type->kind == TY_SHORT || type->kind == TY_INT ||
           type->kind == TY_LONG || type->kind == TY_ENUM;
}

bool is_floatnum(Type *type) {
    return type->kind == TY_FLOAT || type->kind == TY_DOUBLE;
}

bool is_numeric(Type *type) { return is_integer(type) || is_floatnum(type); }

static Type *common_type(Type *ty1, Type *ty2) {
    if (is_pointer(ty1)) {
        return type_ptr(ty1->ptr_to);
    }

    if (ty1->kind == TY_FUNC) return type_ptr(ty1);
    if (ty2->kind == TY_FUNC) return type_ptr(ty2);

    if (ty1->kind == TY_DOUBLE || ty2->kind == TY_DOUBLE) {
        return type_double;
    }
    if (ty1->kind == TY_FLOAT || ty2->kind == TY_FLOAT) {
        return type_float;
    }

    if (ty1->size < 4) ty1 = type_int;
    if (ty2->size < 4) ty2 = type_int;

    if (ty1->size != ty2->size) {
        return (ty1->size < ty2->size) ? ty2 : ty1;
    }

    if (ty2->is_unsigned) return ty2;
    return ty1;
}

static void usual_arith_conv(Node **lhs, Node **rhs) {
    Type *type = common_type((*lhs)->type, (*rhs)->type);
    *lhs = node_cast((*lhs)->tok, type, *lhs);
    *rhs = node_cast((*rhs)->tok, type, *rhs);
}

void set_node_type(Node *node) {
    if (!node || node->type) return;

    set_node_type(node->lhs);
    set_node_type(node->rhs);
    set_node_type(node->cond);
    set_node_type(node->then);
    set_node_type(node->els);
    set_node_type(node->init);
    set_node_type(node->upd);
    for (Node *cur = node->body; cur; cur = cur->next) {
        set_node_type(cur);
    }
    for (Node *cur = node->args; cur; cur = cur->next) {
        set_node_type(cur);
    }

    switch (node->kind) {
        case ND_NUM:
            node->type = type_int;
            return;
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
        case ND_BITAND:
        case ND_BITOR:
        case ND_BITXOR:
            usual_arith_conv(&node->lhs, &node->rhs);
            node->type = node->lhs->type;
            return;
        case ND_NEG: {
            Type *type = common_type(type_int, node->lhs->type);
            node->lhs = node_cast(node->tok, type, node->lhs);
            node->type = type;
            return;
        }
        case ND_ASSIGN:
            if (node->lhs->type->kind == TY_ARRAY) {
                error_tok(node->tok, "配列への代入はできません");
            }
            if (node->lhs->type->kind != TY_STRUCT) {
                node->rhs = node_cast(node->tok, node->lhs->type, node->rhs);
            }
            node->type = node->lhs->type;
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ:
            usual_arith_conv(&node->lhs, &node->rhs);
            node->type = type_int;
            return;
        case ND_NOT:
        case ND_OR:
        case ND_AND:
            node->type = type_int;
            return;
        case ND_BITNOT:
        case ND_BITSHL:
        case ND_BITSHR:
            node->type = node->lhs->type;
            return;
        case ND_CALL:
            node->type = type_long;
            return;
        case ND_COMMA:
            node->type = node->rhs->type;
            return;
        case ND_COND:
            if (node->then->type->kind == TY_VOID ||
                node->els->type->kind == TY_VOID) {
                node->type = type_void;
            } else {
                usual_arith_conv(&node->then, &node->els);
                node->type = node->then->type;
            }
            return;
        case ND_MEMBER:
            node->type = node->member->type;
            return;
        case ND_ADDR: {
            Type *type = node->lhs->type;
            if (type->kind == TY_ARRAY) {
                node->type = type_ptr(type->ptr_to);
            } else {
                node->type = type_ptr(type);
            }
            return;
        }
        case ND_DEREF: {
            Type *ptr_to = node->lhs->type->ptr_to;
            if (ptr_to == NULL) {
                error_tok(node->tok, "無効なポインタの参照");
            }
            if (ptr_to->kind == TY_VOID) {
                error_tok(node->tok, "void 型のポインタの参照");
            }
            node->type = ptr_to;
            return;
        }
        case ND_VAR:
            node->type = node->var->type;
            return;
        case ND_STMT_EXPR:
            if (node->body) {
                Node *stmt = node->body;
                while (stmt->next) {
                    stmt = stmt->next;
                }
                if (stmt->kind == ND_EXPR_STMT) {
                    node->type = stmt->lhs->type;
                    return;
                }
            }
            error_tok(node->tok, "void 値を返すことはできません");
            return;
    }
}
