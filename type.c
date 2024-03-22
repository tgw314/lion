#include <stdlib.h>

#include "lion.h"

Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *new_type_ptr(Type *type) {
    Type *ptr = new_type(TY_PTR);
    ptr->ptr_to = type;
    return ptr;
}

Type *new_type_array(Type *type, size_t size) {
    Type *array = new_type(TY_ARRAY);
    array->ptr_to = type;
    array->array_size = size;
    return array;
}

size_t get_sizeof(Type *type) {
    switch (type->kind) {
        case TY_INT:
            return 4;
        case TY_PTR:
            return 8;
        case TY_ARRAY:
            return type->array_size * get_sizeof(type->ptr_to);
    }
}

void set_expr_type(Node *node) {
    if (!node || node->type) return;

    set_expr_type(node->lhs);
    set_expr_type(node->rhs);
    set_expr_type(node->cond);
    set_expr_type(node->then);
    set_expr_type(node->els);
    set_expr_type(node->init);
    set_expr_type(node->upd);
    for (Node *cur = node->body; cur; cur = cur->next) {
        set_expr_type(cur);
    }
    for (Node *cur = node->args; cur; cur = cur->next) {
        set_expr_type(cur);
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
                error("配列への代入はできません");
            }
            node->type = node->lhs->type;
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ:
        case ND_NUM:
        case ND_CALL:
            node->type = new_type(TY_INT);
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
                error("無効なポインタの参照です");
            }
            node->type = node->lhs->type->ptr_to;
            return;
        case ND_LVAR:
        case ND_GVAR:
            node->type = node->var->type;
            return;
    }
}
