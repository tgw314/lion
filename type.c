#include <stdlib.h>

#include "lion.h"

static Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *num_type(TypeKind kind) {
    static Type char_type = (Type){TY_CHAR, 1, 1};
    static Type short_type = (Type){TY_SHORT, 2, 2};
    static Type int_type = (Type){TY_INT, 4, 4};
    static Type long_type = (Type){TY_LONG, 8, 8};

    switch (kind) {
        case TY_CHAR:
            return &char_type;
        case TY_SHORT:
            return &short_type;
        case TY_INT:
            return &int_type;
        case TY_LONG:
            return &long_type;
        default:
            error("数型ではありません");
    }
}

Type *new_type_func() {
    Type *type = new_type(TY_FUNC);
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

Type *new_type_struct(Member *members) {
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

Type *new_type_union(Member *members) {
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

bool is_pointer(Type *type) {
    return type->kind == TY_PTR || type->kind == TY_ARRAY;
}

bool is_number(Type *type) {
    return type->kind == TY_CHAR || type->kind == TY_SHORT ||
           type->kind == TY_INT || type->kind == TY_LONG;
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
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
            // case ND_NEG:
            node->type = node->lhs->type;
            return;
        case ND_ASSIGN:
            if (node->lhs->type->kind == TY_ARRAY) {
                error_tok(node->tok, "配列への代入はできません");
            }
            node->type = node->lhs->type;
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ:
        case ND_NUM:
        case ND_CALL:
            node->type = num_type(TY_LONG);
            return;
        case ND_COMMA:
            node->type = node->rhs->type;
            return;
        case ND_MEMBER:
            node->type = node->member->type;
            return;
        case ND_ADDR:
            if (node->lhs->type->kind == TY_ARRAY) {
                node->type = new_type_ptr(node->lhs->type->ptr_to);
            } else {
                node->type = new_type_ptr(node->lhs->type);
            }
            return;
        case ND_DEREF:
            if (node->lhs->type->ptr_to == NULL) {
                error_tok(node->tok, "無効なポインタの参照です");
            }
            node->type = node->lhs->type->ptr_to;
            return;
        case ND_LVAR:
        case ND_GVAR:
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
