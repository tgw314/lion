#include <stdarg.h>
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

static void println(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

// align = 2^n の場合のみ有効
int align(int n, int align) { return (n + align - 1) & ~(align - 1); }

typedef enum { I8, I16, I32, I64 } TypeId;

static TypeId type_id(Type *type) {
    switch (type->kind) {
        case TY_CHAR:
            return I8;
        case TY_SHORT:
            return I16;
        case TY_INT:
            return I32;
        case TY_VOID:
            unreachable();
        default:
            return I64;
    }
}

static char *reg_alias(RegAlias64 reg, TypeId id) {
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

    return regs[reg][id];
}

static char *word_ptr(TypeId id) {
    switch (id) {
        case I8:
            return "BYTE PTR";
        case I16:
            return "WORD PTR";
        case I32:
            return "DWORD PTR";
        case I64:
            return "QWORD PTR";
        default:
            unreachable();
    }
}

static void mov_memReg(RegAlias64 dest, RegAlias64 src, Type *type) {
    TypeId id = type_id(type);
    println("  mov %s [%s], %s", word_ptr(id), reg_alias(dest, I64),
            reg_alias(src, id));
}

static void mov_regMem(RegAlias64 dest, RegAlias64 src, Type *type) {
    TypeId id = type_id(type);
    if (id != I64) {
        println("  movsx %s, %s [%s]", reg_alias(dest, I64), word_ptr(id),
                reg_alias(src, I64));
    } else {
        println("  mov %s, %s [%s]", reg_alias(dest, id), word_ptr(id),
                reg_alias(src, I64));
    }
}

static void mov_offsetReg(int offset, RegAlias64 src, Type *type) {
    TypeId id = type_id(type);
    println("  mov %s [rbp%+d], %s", word_ptr(id), offset, reg_alias(src, id));
}

static void call(const char *funcname) {
    int i = count();
    println("  mov rax, rsp");
    println("  and rax, 15");
    println("  jnz .L.call.%s.%03d", funcname, i);
    println("  mov rax, 0");
    println("  call %s", funcname);
    println("  jmp .L.end.%s.%03d", funcname, i);
    println(".L.call.%s.%03d:", funcname, i);
    println("  sub rsp, 8");
    println("  mov rax, 0");
    println("  call %s", funcname);
    println("  add rsp, 8");
    println(".L.end.%s.%03d:", funcname, i);
}

static void cast(Type *from, Type *to) {
    static char i32i8[] = "movsx eax, al";
    static char i32i16[] = "movsx eax, ax";
    static char i32i64[] = "movsxd rax, eax";
    static char *cast_table[][8] = {
        {NULL, NULL, NULL, i32i64},
        {i32i8, NULL, NULL, i32i64},
        {i32i8, i32i16, NULL, i32i64},
        {i32i8, i32i16, NULL, NULL},
    };

    if (to->kind == TY_VOID) return;
    TypeId from_id = type_id(from);
    TypeId to_id = type_id(to);

    if (cast_table[from_id][to_id]) {
        println("  %s", cast_table[from_id][to_id]);
    }
}

static void loc(Node *node) {
    static int line_no = 0;
    if (line_no == node->tok->line_no) {
        return;
    }
    line_no = node->tok->line_no;
    println("  .loc 1 %d", line_no);
}

static void gen_lval(Node *node) {
    switch (node->kind) {
        case ND_LVAR:
            println("  lea rax, [rbp%+d]", node->var->offset);
            return;
        case ND_GVAR:
            println("  lea rax, %s[rip]", node->var->name);
            return;
        case ND_MEMBER:
            gen_lval(node->lhs);
            println("  lea rax, [rax%+d]", node->member->offset);
            return;
        case ND_DEREF:
            gen_expr(node->lhs);
            return;
        case ND_COMMA:
            gen_expr(node->lhs);
            gen_lval(node->rhs);
            return;
        default:
            error_tok(node->tok, "左辺値ではありません");
    }
}

static void gen_expr(Node *node) {
    loc(node);

    switch (node->kind) {
        case ND_NUM:
            println("  mov rax, %ld", node->val);
            return;
        case ND_NEG:
            gen_expr(node->lhs);
            println("  neg rax");
            return;
        case ND_GVAR:
        case ND_LVAR:
        case ND_MEMBER:
            gen_lval(node);
            if (node->type->kind != TY_ARRAY && node->type->kind != TY_STRUCT &&
                node->type->kind != TY_UNION) {
                mov_regMem(RAX, RAX, node->type);
            }
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            println("  push rax");
            gen_expr(node->rhs);
            println("  pop rdi");
            if (node->type->kind == TY_STRUCT || node->type->kind == TY_UNION) {
                for (int i = 0; i < node->type->size; i++) {
                    println("  mov r8b, [rax%+d]", i);
                    println("  mov [rdi%+d], r8b", i);
                }
            } else {
                mov_memReg(RDI, RAX, node->type);
            }
            return;
        case ND_COMMA:
            gen_expr(node->lhs);
            gen_expr(node->rhs);
            return;
        case ND_ADDR:
            gen_lval(node->lhs);
            return;
        case ND_DEREF:
            gen_expr(node->lhs);
            if (node->type->kind != TY_ARRAY && node->type->kind != TY_STRUCT &&
                node->type->kind != TY_UNION) {
                mov_regMem(RAX, RAX, node->type);
            }
            return;
        case ND_CAST:
            gen_expr(node->lhs);
            cast(node->lhs->type, node->type);
            return;
        case ND_CALL: {
            int argc = 0;
            char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

            for (Node *arg = node->args; arg; arg = arg->next) {
                gen_expr(arg);
                println("  push rax");
                argc++;
            }
            for (int i = argc - 1; i >= 0; i--) {
                println("  pop %s", arg_regs[i]);
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
    println("  push rax");
    gen_expr(node->rhs);
    println("  mov rdi, rax");
    println("  pop rax");

    Type *t = node->lhs->type;
    TypeId id = t->kind == TY_LONG || t->ptr_to ? I64 : I32;
    char *rax = reg_alias(RAX, id);
    char *rdi = reg_alias(RDI, id);

    switch (node->kind) {
        case ND_ADD:
            println("  add %s, %s", rax, rdi);
            return;
        case ND_SUB:
            println("  sub %s, %s", rax, rdi);
            return;
        case ND_MUL:
            println("  imul %s, %s", rax, rdi);
            return;
        case ND_DIV:
            if (id == I64) {
                println("  cqo");
            } else {
                println("  cdq");
            }
            println("  idiv %s", rdi);
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ:
            println("  cmp %s, %s", rax, rdi);
            switch (node->kind) {
                case ND_EQ:
                    println("  sete al");
                    break;
                case ND_NEQ:
                    println("  setne al");
                    break;
                case ND_LS:
                    println("  setl al");
                    break;
                case ND_LEQ:
                    println("  setle al");
                    break;
            }
            println("  movzb rax, al");
            return;
    }
}

static void gen_stmt(Node *node) {
    loc(node);

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
            println("  cmp rax, 0");
            println("  je  .L.else.%03d", i);
            gen_stmt(node->then);
            println("  jmp .L.end.%03d", i);
            println(".L.else.%03d:", i);
            if (node->els) {
                gen_stmt(node->els);
            }
            println(".L.end.%03d:", i);
            return;
        }
        case ND_WHILE: {
            int i = count();
            println(".L.begin.%03d:", i);
            gen_expr(node->cond);
            println("  cmp rax, 0");
            println("  je  .L.end.%03d", i);
            gen_stmt(node->then);
            println("  jmp .L.begin.%03d", i);
            println(".L.end.%03d:", i);
            return;
        }
        case ND_FOR: {
            int i = count();
            if (node->init) {
                gen_expr(node->init);
            }
            println(".L.begin.%03d:", i);
            if (node->cond) {
                gen_expr(node->cond);
                println("  cmp rax, 0");
                println("  je  .L.end.%03d", i);
            }
            gen_stmt(node->then);
            if (node->upd) {
                gen_expr(node->upd);
            }
            println("  jmp .L.begin.%03d", i);
            println(".L.end.%03d:", i);
            return;
        }
        case ND_RETURN:
            gen_expr(node->lhs);
            println("  jmp .L.return.%s", obj->name);
            return;
    }

    error_tok(node->tok, "不正な文です");
}

void generate(Object *globals) {
    RegAlias64 param_regs[] = {RDI, RSI, RDX, RCX, R8, R9};

    println(".intel_syntax noprefix");
    for (obj = globals; obj; obj = obj->next) {
        if (!obj->is_func) {
            println(".data");
            println(".globl %s", obj->name);
            println("%s:", obj->name);

            if (obj->init_data) {
                if (!is_number(obj->type)) {
                    for (int i = 0; i < obj->type->array_size; i++) {
                        println("  .byte %d", obj->init_data[i]);
                    }
                }
            } else {
                println("  .zero %d", (int)obj->type->size);
            }

            continue;
        }
        if (!obj->is_def) continue;

        {  // ローカル変数のオフセットを計算
            int stack_size = 0;
            for (Object *v = obj->locals; v; v = v->next) {
                stack_size += v->type->size;
                stack_size = align(stack_size, v->type->align);
                v->offset = -stack_size;
            }
            obj->stack_size = align(stack_size, 16);
        }

        println(".text");
        println(".globl %s", obj->name);
        println("%s:", obj->name);

        // プロローグ
        println("  push rbp");
        println("  mov rbp, rsp");
        if (obj->stack_size > 0) {
            println("  sub rsp, %d", obj->stack_size);
        }

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
        println(".L.return.%s:", obj->name);
        println("  mov rsp, rbp");
        println("  pop rbp");
        println("  ret");
    }
}
