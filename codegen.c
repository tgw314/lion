#include <stdio.h>

#include "lion.h"

typedef enum {
    RAX,
    RDI,
    RSI,
    RDX,
    RCX,
    RBP,
    RSP,
    RBX,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
} RegAlias64;

static Object *obj;

static void gen_lval(Node *node);
static void gen_expr(Node *node);
static void gen_stmt(Node *node);

static int count() {
    static int i = 0;
    return i++;
}

// align = 2^n の場合のみ有効
static int align(int n, int align) { return (n + align - 1) & ~(align - 1); }

static char *reg_alias(RegAlias64 reg, size_t size) {
    static char *regs[16][4] = {
        {"al", "ax", "eax", "rax"},      {"dil", "di", "edi", "rdi"},
        {"sil", "si", "esi", "rsi"},     {"dl", "dx", "edx", "rdx"},
        {"cl", "cx", "ecx", "rcx"},      {"bpl", "bp", "ebp", "rbp"},
        {"spl", "sp", "esp", "rsp"},     {"bl", "bx", "ebx", "rbx"},
        {"r8b", "r8w", "r8d", "r8"},     {"r9b", "r9w", "r9d", "r9"},
        {"r10b", "r10w", "r10d", "r10"}, {"r11b", "r11w", "r11d", "r11"},
        {"r12b", "r12w", "r12d", "r12"}, {"r13b", "r13w", "r13d", "r13"},
        {"r14b", "r14w", "r14d", "r14"}, {"r15b", "r15w", "r15d", "r15"},
    };

    int idx;
    switch (size) {
        case 1:
            idx = 0;
            break;
        case 2:
            idx = 1;
            break;
        case 4:
            idx = 2;
            break;
        case 8:
            idx = 3;
            break;
        default:
            error("不正なサイズです");
    }

    return regs[reg][idx];
}

static char *word_ptr(size_t size) {
    switch (size) {
        case 1:
            return "BYTE PTR";
        case 2:
            return "WORD PTR";
        case 4:
            return "DWORD PTR";
        case 8:
            return "QWORD PTR";
        default:
            error("不正なサイズです");
    }
}

static void mov_memReg(RegAlias64 dest, RegAlias64 src, Type *type) {
    size_t size = get_sizeof(type);
    printf("  mov %s [%s], %s\n", word_ptr(size), reg_alias(dest, 8),
           reg_alias(src, size));
}

static void mov_regMem(RegAlias64 dest, RegAlias64 src, Type *type) {
    size_t size = get_sizeof(type);
    if (size == 1) {
        printf("  movsx %s, %s [%s]\n", reg_alias(dest, 8), word_ptr(size),
               reg_alias(src, 8));
    } else {
        printf("  mov %s, %s [%s]\n", reg_alias(dest, size), word_ptr(size),
               reg_alias(src, 8));
    }
}

static void mov_regOffset(RegAlias64 dest, int offset, Type *type) {
    size_t size = get_sizeof(type);
    printf("  mov %s, %s [rbp%+d]\n", reg_alias(dest, size), word_ptr(size),
           offset);
}

static void mov_offsetReg(int offset, RegAlias64 src, Type *type) {
    size_t size = get_sizeof(type);
    printf("  mov %s [rbp%+d], %s\n", word_ptr(size), offset,
           reg_alias(src, size));
}

static void call(const char *funcname) {
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
}

static void gen_lval(Node *node) {
    switch (node->kind) {
        case ND_LVAR:
            printf("  lea rax, [rbp%+d]\n", node->var->offset);
            return;
        case ND_GVAR:
            printf("  lea rax, %s[rip]\n", node->var->name);
            return;
        case ND_DEREF:
            gen_expr(node->lhs);
            return;
        default:
            error("代入の左辺値が変数ではありません");
    }
}

static void gen_expr(Node *node) {
    switch (node->kind) {
        case ND_NUM:
            printf("  mov rax, %d\n", node->val);
            return;
        case ND_GVAR:
        case ND_LVAR:
            gen_lval(node);
            if (node->type->kind != TY_ARRAY) {
                mov_regMem(RAX, RAX, node->type);
            }
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            printf("  push rax\n");
            gen_expr(node->rhs);
            printf("  pop rdi\n");
            mov_memReg(RDI, RAX, node->type);
            return;
        case ND_ADDR:
            gen_lval(node->lhs);
            return;
        case ND_DEREF:
            gen_expr(node->lhs);
            if (node->type->kind != TY_ARRAY) {
                mov_regMem(RAX, RAX, node->type);
            }
            return;
        case ND_CALL: {
            int argc = 0;
            char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

            for (Node *arg = node->args; arg; arg = arg->next) {
                gen_expr(arg);
                printf("  push rax\n");
                argc++;
            }
            for (int i = argc - 1; i >= 0; i--) {
                printf("  pop %s\n", arg_regs[i]);
            }
            call(node->funcname);
            return;
        }
        case ND_STMT_EXPR:
            for (Node *n = node->body; n; n = n->next) {
                gen_stmt(n);
            }
            return;
    }

    gen_expr(node->lhs);
    printf("  push rax\n");
    gen_expr(node->rhs);
    printf("  mov rdi, rax\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NEQ:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LS:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LEQ:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }
}

static void gen_stmt(Node *node) {
    switch (node->kind) {
        case ND_EXPR_STMT:
            gen_expr(node->lhs);
            return;
        case ND_BLOCK:
            for (Node *n = node->body; n; n = n->next) {
                gen_stmt(n);
            }
            return;
        case ND_IF: {
            int i = count();
            gen_expr(node->cond);
            printf("  cmp rax, 0\n");
            printf("  je  .L.else.%03d\n", i);
            gen_stmt(node->then);
            printf("  jmp .L.end.%03d\n", i);
            printf(".L.else.%03d:\n", i);
            if (node->els) {
                gen_stmt(node->els);
            }
            printf(".L.end.%03d:\n", i);
            return;
        }
        case ND_WHILE: {
            int i = count();
            printf(".L.begin.%03d:\n", i);
            gen_expr(node->cond);
            printf("  cmp rax, 0\n");
            printf("  je  .L.end.%03d\n", i);
            gen_stmt(node->then);
            printf("  jmp .L.begin.%03d\n", i);
            printf(".L.end.%03d:\n", i);
            return;
        }
        case ND_FOR: {
            int i = count();
            if (node->init) {
                gen_expr(node->init);
            }
            printf(".L.begin.%03d:\n", i);
            if (node->cond) {
                gen_expr(node->cond);
                printf("  cmp rax, 0\n");
                printf("  je  .L.end.%03d\n", i);
            }
            gen_stmt(node->then);
            if (node->upd) {
                gen_expr(node->upd);
            }
            printf("  jmp .L.begin.%03d\n", i);
            printf(".L.end.%03d:\n", i);
            return;
        }
        case ND_RETURN:
            gen_expr(node->lhs);
            printf("  jmp .L.return.%s\n", obj->name);
            return;
    }

    error("不正な文です");
}

void generate(Object *globals) {
    RegAlias64 param_regs[] = {RDI, RSI, RDX, RCX, R8, R9};

    printf(".intel_syntax noprefix\n");
    for (obj = globals; obj; obj = obj->next) {
        if (!obj->is_func) {
            printf(".data\n");
            printf(".globl %s\n", obj->name);
            printf("%s:\n", obj->name);

            if (obj->init_data) {
                if (!is_number(obj->type)) {
                    for (int i = 0; i < obj->type->array_size; i++) {
                        printf("  .byte %d\n", obj->init_data[i]);
                    }
                }
            } else {
                printf("  .zero %d\n", (int)get_sizeof(obj->type));
            }

            continue;
        }

        {  // ローカル変数のオフセットを計算
            int offset = 0;
            for (Object *v = obj->locals; v; v = v->next) {
                offset -= get_sizeof(v->type);
                v->offset = offset;
            }
            obj->stack_size = -offset;
        }

        printf(".text\n");
        printf(".globl %s\n", obj->name);
        printf("%s:\n", obj->name);

        // プロローグ
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        // 予めアラインしているので以降は無視できる
        printf("  sub rsp, %d\n", align(obj->stack_size, 16));

        {  // 引数をローカル変数として代入
            int i = 0;
            for (Object *p = obj->params; p; p = p->next) {
                mov_offsetReg(p->offset, param_regs[i++], p->type);
            }
        }

        // 先頭の式から順にコード生成
        for (Node *s = obj->body; s; s = s->next) {
            gen_stmt(s);
        }

        // エピローグ
        // 最後の式の結果が RAX に残っているのでそれが返り値になる
        printf(".L.return.%s:\n", obj->name);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}
