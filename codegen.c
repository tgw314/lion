#include <stdio.h>

#include "lion.h"

static Function *func;
static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static void gen_lval(Node *node);
static void gen_stmt(Node *node);

static int count() {
    static int i = 0;
    return i++;
}

// align = 2^n の場合のみ有効
static int align(int n, int align) { return (n + align - 1) & ~(align - 1); }

static void call(const char *funcname) {
    // if (rsp_offset % 16) {
    //     printf("  sub rsp, 8\n");
    // }
    int i = count();
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%s.%03d\n", funcname, i);
    printf("  mov rax, 0\n");
    printf("  call %s\n", funcname);
    printf("  jmp .L.end.%s.%03d\n", funcname, i);
    printf(".L.call.%s.%03d:\n", funcname, i);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", funcname);
    printf("  add rsp, 8\n");
    printf(".L.end.%s.%03d:\n", funcname, i);
    printf("  push rax\n");
}

static void gen_lval(Node *node) {
    if (node->kind == ND_LVAR) {
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", node->lvar->offset);
        printf("  push rax\n");
        return;
    }
    if (node->kind == ND_DEREF) {
        gen_stmt(node->lhs);
        return;
    }
    error("代入の左辺値が変数ではありません");
}

static void gen_stmt(Node *node) {
    switch (node->kind) {
        case ND_NUM:
            puts("# ND_NUM {");
            printf("  push %d\n", node->val);
            puts("# } ND_NUM");
            return;
        case ND_LVAR:
            puts("# ND_LVAR {");
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            puts("# } ND_LVAR");
            return;
        case ND_ASSIGN:
            puts("# ND_ASSIGN {");
            gen_lval(node->lhs);
            gen_stmt(node->rhs);
            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            puts("# } ND_ASSIGN");
            return;
        case ND_BLOCK:
            puts("# ND_BLOCK {");
            for (Node *n = node->body; n; n = n->next) {
                gen_stmt(n);
            }
            puts("# } ND_BLOCK");
            return;
        case ND_IF: {
            int i = count();
            puts("# ND_IF {");
            gen_stmt(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .L.else.%03d\n", i);
            gen_stmt(node->then);
            printf("  jmp .L.end.%03d\n", i);
            printf(".L.else.%03d:\n", i);
            if (node->els) {
                gen_stmt(node->els);
            }
            printf(".L.end.%03d:\n", i);
            puts("# } ND_IF");
            return;
        }
        case ND_WHILE: {
            int i = count();
            puts("# ND_WHILE {");
            printf(".L.begin.%03d:\n", i);
            gen_stmt(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .L.end.%03d\n", i);
            gen_stmt(node->then);
            printf("  jmp .L.begin.%03d\n", i);
            printf(".L.end.%03d:\n", i);
            puts("# } ND_WHILE");
            return;
        }
        case ND_FOR: {
            int i = count();
            puts("# ND_FOR {");
            if (node->init) {
                gen_stmt(node->init);
            }
            printf(".L.begin.%03d:\n", i);
            if (node->cond) {
                gen_stmt(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .L.end.%03d\n", i);
            }
            gen_stmt(node->then);
            if (node->upd) {
                gen_stmt(node->upd);
            }
            printf("  jmp .L.begin.%03d\n", i);
            printf(".L.end.%03d:\n", i);
            puts("# } ND_FOR");
            return;
        }
        case ND_CALL: {
            int argc = 0;
            puts("# ND_CALL {");
            for (Node *arg = node->args; arg; arg = arg->next) {
                gen_stmt(arg);
                argc++;
            }
            for (int i = argc - 1; i >= 0; i--) {
                printf("  pop %s\n", arg_regs[i]);
            }
            call(node->funcname);
            puts("# } ND_CALL");
            return;
        }
        case ND_ADDR:
            puts("# ND_ADDR {");
            gen_lval(node->lhs);
            puts("# } ND_ADDR");
            return;
        case ND_DEREF:
            puts("# ND_DEREF {");
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            puts("# } ND_DEREF");
            return;
        case ND_RETURN:
            puts("# ND_RETURN {");
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  jmp .L.return.%s\n", func->name);
            puts("# } ND_RETURN");
            return;
    }

    gen_stmt(node->lhs);
    gen_stmt(node->rhs);

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

void generate(Function *funcs) {
    printf(".intel_syntax noprefix\n");
    for (func = funcs; func; func = func->next) {
        printf(".globl %s\n", func->name);
        printf("%s:\n", func->name);

        // プロローグ
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        // 予めアラインしているので以降は無視できる
        printf("  sub rsp, %d\n", align(func->stack_size, 16));

        // 引数をローカル変数として代入
        int i = 0;
        for (LVar *arg = func->locals; arg != NULL && arg->kind == LV_ARG;
             arg = arg->next) {
            int offset = arg->offset;
            printf("  mov rax, rbp\n");
            printf("  sub rax, %d\n", offset);
            printf("  mov [rax], %s\n", arg_regs[i++]);
        }

        // 先頭の式から順にコード生成
        for (Node *s = func->body; s; s = s->next) {
            gen_stmt(s);
        }

        // エピローグ
        // 最後の式の結果が RAX に残っているのでそれが返り値になる
        printf(".L.return.%s:\n", func->name);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}
