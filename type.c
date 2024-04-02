#include <stdlib.h>

#include "lion.h"

static size_t get_sizeof(Type *type) {
    if (type->size) return type->size;

    switch (type->kind) {
        case TY_CHAR:
            return type->size = 1;
        case TY_INT:
            return type->size = 4;
        case TY_PTR:
            return type->size = 8;
        case TY_ARRAY:
            return type->size = type->array_size * get_sizeof(type->ptr_to);
        case TY_STRUCT:
            for (Member *mem = type->members; mem; mem = mem->next) {
                type->size += get_sizeof(mem->type);
            }
            return type->size;
    }
}

static Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *new_type_num(TypeKind kind) {
    Type *type = new_type(kind);
    if (!is_number(type)) {
        error("数型ではありません");
    }

    type->size = get_sizeof(type);
    type->align = type->size;
    return type;
}

Type *new_type_func() {
    Type *type = new_type(TY_FUNC);
    return type;
}

Type *new_type_ptr(Type *base_type) {
    Type *type = new_type(TY_PTR);
    type->ptr_to = base_type;

    type->size = get_sizeof(type);
    type->align = type->size;
    return type;
}

Type *new_type_array(Type *base_type, size_t size) {
    Type *type = new_type(TY_ARRAY);
    type->ptr_to = base_type;
    type->array_size = size;

    type->size = get_sizeof(type);
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
        offset += get_sizeof(mem->type);

        if (type->align < mem->type->align) {
            type->align = mem->type->align;
        }
    }
    type->size = align(offset, type->align);

    return type;
}

bool is_pointer(Type *type) {
    return type->kind == TY_PTR || type->kind == TY_ARRAY;
}

bool is_number(Type *type) {
    return type->kind == TY_INT || type->kind == TY_CHAR;
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
            node->type = new_type(TY_INT);
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
