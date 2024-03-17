#include <stdlib.h>

#include "lion.h"

Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = kind;
    switch (type->kind) {
        case TY_INT:
            type->size = 4;
            break;
        case TY_PTR:
            type->size = 8;
            break;
    }
    return type;
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
        case ND_ASSIGN:
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
            node->type = new_type(TY_PTR);
            node->type->ptr_to = node->lhs->type;
            return;
        case ND_DEREF:
            if (node->lhs->type->kind != TY_PTR) {
                error("無効なポインタの参照です");
            }
            node->type = node->lhs->type->ptr_to;
            return;
        case ND_LVAR:
            node->type = node->lvar->type;
            return;
    }
}
