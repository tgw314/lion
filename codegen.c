#include <stdio.h>

#include "lion.h"

static int count() {
    static int i = 0;
    return i++;
}

static void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NUM:
            puts("# ND_NUM {");
            printf("  push %d\n", node->val);
            puts("# } ND_NUM");
            return;
        case ND_LVAR:
            gen_lval(node);
            puts("# ND_LVAR {");
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            puts("# } ND_LVAR");
            return;
        case ND_ASSIGN:
            puts("# ND_ASSIGN {");
            gen_lval(node->lhs);
            gen(node->rhs);
            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            puts("# } ND_ASSIGN");
            return;
        case ND_BLOCK:
            puts("# ND_BLOCK {");
            for (Node *n = node->body; n; n = n->next) {
                gen(n);
            }
            puts("# } ND_BLOCK");
            return;
        case ND_IF: {
            int i = count();
            puts("# ND_IF {");
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lelse%03d\n", i);
            gen(node->then);
            printf("  jmp .Lend%03d\n", i);
            printf(".Lelse%03d:\n", i);
            if (node->els) {
                gen(node->els);
            }
            printf(".Lend%03d:\n", i);
            puts("# } ND_IF");
            return;
        }
        case ND_WHILE: {
            int i = count();
            puts("# ND_WHILE {");
            printf(".Lbegin%03d:\n", i);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%03d\n", i);
            gen(node->then);
            printf("  jmp .Lbegin%03d\n", i);
            printf(".Lend%03d:\n", i);
            puts("# } ND_WHILE");
            return;
        }
        case ND_FOR: {
            int i = count();
            puts("# ND_FOR {");
            if (node->init) {
                gen(node->init);
            }
            printf(".Lbegin%03d:\n", i);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%03d\n", i);
            gen(node->then);
            if (node->upd) {
                gen(node->upd);
            }
            printf("  jmp .Lbegin%03d\n", i);
            printf(".Lend%03d:\n", i);
            puts("# } ND_FOR");
            return;
        }
        case ND_RETURN:
            puts("# ND_RETURN {");
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            printf("  ret\n");
            puts("# } ND_RETURN");
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    puts("# {");
    printf("  pop rdi\n");
    printf("  pop rax\n");
    puts("# }");

    switch (node->kind) {
        case ND_ADD:
            puts("# ND_ADD {");
            printf("  add rax, rdi\n");
            puts("# } ND_ADD");
            break;
        case ND_SUB:
            puts("# ND_SUB {");
            printf("  sub rax, rdi\n");
            puts("# } ND_SUB");
            break;
        case ND_MUL:
            puts("# ND_MUL {");
            printf("  imul rax, rdi\n");
            puts("# } ND_MUL");
            break;
        case ND_DIV:
            puts("# ND_DIV {");
            printf("  cqo\n");
            printf("  idiv rdi\n");
            puts("# } ND_DIV");
            break;
        case ND_EQ:
            puts("# ND_EQ {");
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            puts("# } ND_EQ");
            break;
        case ND_NEQ:
            puts("# ND_NEQ {");
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            puts("# } ND_NEQ");
            break;
        case ND_LS:
            puts("# ND_LS {");
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            puts("# } ND_LS");
            break;
        case ND_LEQ:
            puts("# ND_LEQ {");
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            puts("# } ND_LEQ");
            break;
    }

    printf("  push rax\n");
}
