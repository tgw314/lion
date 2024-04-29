#include <stdlib.h>

#include "lion.h"

static Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *basic_type(TypeKind kind) {
    static Type void_type = (Type){TY_VOID, 1, 1},
                bool_type = (Type){TY_BOOL, 1, 1},
                char_type = (Type){TY_CHAR, 1, 1},
                short_type = (Type){TY_SHORT, 2, 2},
                int_type = (Type){TY_INT, 4, 4},
                long_type = (Type){TY_LONG, 8, 8};

    switch (kind) {
        case TY_VOID: return &void_type;
        case TY_BOOL: return &bool_type;
        case TY_CHAR: return &char_type;
        case TY_SHORT: return &short_type;
        case TY_INT: return &int_type;
        case TY_LONG: return &long_type;
        default: unreachable();
    }
}

Type *new_type_func(Type *return_type, Type *params) {
    Type *type = new_type(TY_FUNC);
    type->return_type = return_type;
    type->params = params;
    return type;
}

Type *new_type_enum() {
    Type *type = new_type(TY_ENUM);
    type->size = type->align = 4;
    return type;
}

Type *new_type_ptr(Type *base_type) {
    Type *type = new_type(TY_PTR);
    type->ptr_to = base_type;

    type->size = 8;
    type->align = type->size;
    return type;
}

Type *new_type_array(Type *base_type, size_t size) {
    Type *type = new_type(TY_ARRAY);
    type->ptr_to = base_type;
    type->array_size = size;

    type->size = size * base_type->size;
    type->align = base_type->align;
    return type;
}

static Type *new_type_struct(Member *members) {
    Type *type = new_type(TY_STRUCT);
    type->members = members;
    type->align = 1;

    int offset = 0;
    for (Member *mem = type->members; mem; mem = mem->next) {
        offset = align(offset, mem->type->align);
        mem->offset = offset;
        offset += mem->type->size;

        if (type->align < mem->type->align) {
            type->align = mem->type->align;
        }
    }
    type->size = align(offset, type->align);

    return type;
}

static Type *new_type_union(Member *members) {
    Type *type = new_type(TY_UNION);
    type->members = members;
    type->align = 1;

    for (Member *mem = type->members; mem; mem = mem->next) {
        if (type->align < mem->type->align) {
            type->align = mem->type->align;
        }
        if (type->size < mem->type->size) {
            type->size = mem->type->size;
        }
    }
    type->size = align(type->size, type->align);

    return type;
}

Type *new_type_struct_union(TypeKind kind, Member *members) {
    if (kind == TY_STRUCT) {
        return new_type_struct(members);
    }
    if (kind == TY_UNION) {
        return new_type_union(members);
    }
    unreachable();
}

bool is_pointer(Type *type) { return type->ptr_to != NULL; }

bool is_number(Type *type) {
    return type->kind == TY_BOOL || type->kind == TY_CHAR ||
           type->kind == TY_SHORT || type->kind == TY_INT ||
           type->kind == TY_LONG || type->kind == TY_ENUM;
}

static Type *common_type(Type *ty1, Type *ty2) {
    if (is_pointer(ty1)) {
        return new_type_ptr(ty1->ptr_to);
    }
    if (ty1->size == 8 || ty2->size == 8) {
        return basic_type(TY_LONG);
    }
    return basic_type(TY_INT);
}

static void usual_arith_conv(Node **lhs, Node **rhs) {
    Type *type = common_type((*lhs)->type, (*rhs)->type);
    *lhs = new_node_cast((*lhs)->tok, type, *lhs);
    *rhs = new_node_cast((*rhs)->tok, type, *rhs);
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
            node->type = (node->val == (int)node->val) ? basic_type(TY_INT)
                                                       : basic_type(TY_LONG);
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
            Type *type = common_type(basic_type(TY_INT), node->lhs->type);
            node->lhs = new_node_cast(node->tok, type, node->lhs);
            node->type = type;
            return;
        }
        case ND_ASSIGN:
            if (node->lhs->type->kind == TY_ARRAY) {
                error_tok(node->tok, "配列への代入はできません");
            }
            if (node->lhs->type->kind != TY_STRUCT) {
                node->rhs =
                    new_node_cast(node->tok, node->lhs->type, node->rhs);
            }
            node->type = node->lhs->type;
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ:
            usual_arith_conv(&node->lhs, &node->rhs);
            node->type = basic_type(TY_INT);
            return;
        case ND_NOT:
        case ND_OR:
        case ND_AND: node->type = basic_type(TY_INT); return;
        case ND_BITNOT:
        case ND_BITSHL:
        case ND_BITSHR: node->type = node->lhs->type; return;
        case ND_CALL: node->type = basic_type(TY_LONG); return;
        case ND_COMMA: node->type = node->rhs->type; return;
        case ND_COND:
            if (node->then->type->kind == TY_VOID ||
                node->els->type->kind == TY_VOID) {
                node->type = basic_type(TY_VOID);
            } else {
                usual_arith_conv(&node->then, &node->els);
                node->type = node->then->type;
            }
            return;
        case ND_MEMBER: node->type = node->member->type; return;
        case ND_ADDR:
            if (node->lhs->type->kind == TY_ARRAY) {
                node->type = new_type_ptr(node->lhs->type->ptr_to);
            } else {
                node->type = new_type_ptr(node->lhs->type);
            }
            return;
        case ND_DEREF:
            if (node->lhs->type->ptr_to == NULL) {
                error_tok(node->tok, "無効なポインタの参照");
            }
            if (node->lhs->type->ptr_to->kind == TY_VOID) {
                error_tok(node->tok, "void 型のポインタの参照");
            }
            node->type = node->lhs->type->ptr_to;
            return;
        case ND_VAR: node->type = node->var->type; return;
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
