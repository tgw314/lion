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
static int offset = 0;

static void gen_lval(Node *node);
static void gen_expr(Node *node);
static void gen_stmt(Node *node);

static int count(void) {
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

int align(int n, int align) {
    if (align == 0) return n;
    return (n + align - 1) / align * align;
}

typedef enum { I8, I16, I32, I64, U8, U16, U32, U64 } TypeId;

static TypeId type_id(Type *type) {
    switch (type->kind) {
            // clang-format off
        case TY_BOOL:
        case TY_CHAR:  return type->is_unsigned ? U8 : I8;
        case TY_SHORT: return type->is_unsigned ? U16 : I16;
        case TY_INT:   return type->is_unsigned ? U32 : I32;
        case TY_LONG:  return type->is_unsigned ? U64 : I64;
        case TY_VOID: unreachable();
        default: return U64;
            // clang-format on
    }
}

static char *reg(RegAlias64 reg, TypeId id) {
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

    return regs[reg][id % 4];
}

static char *word(TypeId id) {
    switch (id) {
        case I8:
        case U8: return "BYTE PTR";
        case I16:
        case U16: return "WORD PTR";
        case I32:
        case U32: return "DWORD PTR";
        case I64:
        case U64: return "QWORD PTR";
        default: unreachable();
    }
}

static void push(char *r) {
    println("  push %s", r);
    offset += 8;
}

static void pop(const char *r) {
    println("  pop %s", r);
    offset -= 8;
}

static void load(Type *type) {
    if (type->kind == TY_ARRAY || type->kind == TY_STRUCT ||
        type->kind == TY_UNION) {
        return;
    }

    TypeId id = type_id(type);

    switch (id) {
        case I8:
        case I16:
        case U8:
        case U16:
        case U32:
            println("  mov%cx %s, %s [%s]", type->is_unsigned ? 'z' : 's',
                    reg(RAX, I32), word(id), reg(RAX, I64));
            return;
        case I32:
            println("  movsxd %s, %s [%s]", reg(RAX, I64), word(id),
                    reg(RAX, I64));
            return;
        case U64:
        case I64:
            println("  mov %s, %s [%s]", reg(RAX, I64), word(id),
                    reg(RAX, I64));
            return;
    }
}

static void cast(Type *from, Type *to) {
    static char i32i8[] = "movsx eax, al";
    static char i32u8[] = "movzx eax, al";
    static char i32i16[] = "movsx eax, ax";
    static char i32u16[] = "movzx eax, ax";
    static char i32i64[] = "movsxd rax, eax";
    static char u32i64[] = "mov eax, eax";
    static char *cast_table[][8] = {
        // clang-format off
        // i8   i16     i32   i64     u8     u16     u32   u64
        {NULL,  NULL,   NULL, i32i64, i32u8, i32u16, NULL, i32i64},   // i8
        {i32i8, NULL,   NULL, i32i64, i32u8, i32u16, NULL, i32i64},   // i16
        {i32i8, i32i16, NULL, i32i64, i32u8, i32u16, NULL, i32i64},   // i32
        {i32i8, i32i16, NULL, NULL,   i32u8, i32u16, NULL, NULL},     // i64
        {i32i8, NULL,   NULL, i32i64, NULL,  NULL,   NULL, i32i64},   // u8
        {i32i8, i32i16, NULL, i32i64, i32u8, NULL,   NULL, i32i64},   // u16
        {i32i8, i32i16, NULL, u32i64, i32u8, i32u16, NULL, u32i64},   // u32
        {i32i8, i32i16, NULL, NULL,   i32u8, i32u16, NULL, NULL},     // u64
        // clang-format on
    };

    if (to->kind == TY_VOID) return;

    TypeId from_id = type_id(from);
    TypeId to_id = type_id(to);

    if (to->kind == TY_BOOL) {
        char *reg = (from_id <= I32) ? "eax" : "rax";
        println("  cmp %s, 0", reg);
        println("  setne al");
        println("  movzx eax, al");
        return;
    }

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
        case ND_VAR:
            if (node->var->is_local) {
                println("  lea rax, [rbp%+d]", node->var->offset);
            } else {
                println("  lea rax, %s[rip]", node->var->name);
            }
            return;
        case ND_MEMBER:
            gen_lval(node->lhs);
            println("  lea rax, [rax%+d]", node->member->offset);
            return;
        case ND_DEREF: gen_expr(node->lhs); return;
        case ND_COMMA:
            gen_expr(node->lhs);
            gen_lval(node->rhs);
            return;
        default: error_tok(node->tok, "左辺値ではありません");
    }
}

static void gen_expr(Node *node) {
    loc(node);

    switch (node->kind) {
        case ND_NULL_EXPR: return;
        case ND_NUM: println("  mov rax, %ld", node->val); return;
        case ND_NEG:
            gen_expr(node->lhs);
            println("  neg rax");
            return;
        case ND_NOT:
            gen_expr(node->lhs);
            println("  cmp rax, 0");
            println("  sete al");
            println("  movzx rax, al");
            return;
        case ND_BITNOT:
            gen_expr(node->lhs);
            println("  not rax");
            return;
        case ND_AND: {
            int c = count();
            gen_expr(node->lhs);
            println("  cmp rax, 0");
            println("  je .L.false.%d", c);
            gen_expr(node->rhs);
            println("  cmp rax, 0");
            println("  je .L.false.%d", c);
            println("  mov rax, 1");
            println("  jmp .L.end.%d", c);
            println(".L.false.%d:", c);
            println("  mov rax, 0");
            println(".L.end.%d:", c);
            return;
        }
        case ND_OR: {
            int c = count();
            gen_expr(node->lhs);
            println("  cmp rax, 0");
            println("  jne .L.true.%d", c);
            gen_expr(node->rhs);
            println("  cmp rax, 0");
            println("  jne .L.true.%d", c);
            println("  mov rax, 0");
            println("  jmp .L.end.%d", c);
            println(".L.true.%d:", c);
            println("  mov rax, 1");
            println(".L.end.%d:", c);
            return;
        }
        case ND_VAR:
        case ND_MEMBER:
            gen_lval(node);
            load(node->type);
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            push("rax");
            gen_expr(node->rhs);
            pop("rdi");
            if (node->type->kind == TY_STRUCT || node->type->kind == TY_UNION) {
                for (int i = 0; i < node->type->size; i++) {
                    println("  mov r8b, [rax%+d]", i);
                    println("  mov [rdi%+d], r8b", i);
                }
            } else {
                TypeId id = type_id(node->type);
                println("  mov %s [%s], %s", word(id), reg(RDI, I64),
                        reg(RAX, id));
            }
            return;
        case ND_MEMZERO:
            println("  mov rcx, %d", node->var->type->size);
            println("  lea rdi, [rbp%+d]", node->var->offset);
            println("  mov al, 0");
            println("  rep stosb");
            return;
        case ND_COMMA:
            gen_expr(node->lhs);
            gen_expr(node->rhs);
            return;
        case ND_COND: {
            int c = count();
            gen_expr(node->cond);
            println("  cmp eax, 0");
            println("  je .L.else.%03d", c);
            gen_expr(node->then);
            println("  jmp .L.end.%03d", c);
            println(".L.else.%03d:", c);
            gen_expr(node->els);
            println(".L.end.%03d:", c);
            return;
        }
        case ND_ADDR: gen_lval(node->lhs); return;
        case ND_DEREF:
            gen_expr(node->lhs);
            load(node->type);
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
                push("rax");
                argc++;
            }
            for (int i = argc - 1; i >= 0; i--) pop(arg_regs[i]);

            if (offset % 16) {
                println("  sub rsp, 8");
                println("  call %s", node->funcname);
                println("  add rsp, 8");
            } else {
                println("  call %s", node->funcname);
            }

            bool is_u = node->type->is_unsigned;
            switch (node->type->kind) {
                case TY_BOOL: println("  movzx eax, al"); return;
                case TY_CHAR:
                    println("  mov%cx eax, al", is_u ? 'z' : 's');
                    return;
                case TY_SHORT:
                    println("  mov%cx eax, ax", is_u ? 'z' : 's');
                    return;
            }
            return;
        }
        case ND_STMT_EXPR:
            for (Node *n = node->body; n; n = n->next) gen_stmt(n);
            return;
    }

    gen_expr(node->lhs);
    push("rax");
    gen_expr(node->rhs);
    println("  mov rdi, rax");
    pop("rax");

    Type *t = node->lhs->type;
    TypeId id = t->kind == TY_LONG || t->ptr_to ? I64 : I32;
    char *rax = reg(RAX, id);
    char *rdi = reg(RDI, id);
    char *rdx = reg(RDX, id);

    switch (node->kind) {
        case ND_ADD: println("  add %s, %s", rax, rdi); return;
        case ND_SUB: println("  sub %s, %s", rax, rdi); return;
        case ND_MUL: println("  imul %s, %s", rax, rdi); return;
        case ND_MOD:
        case ND_DIV:
            if (node->type->is_unsigned) {
                println("  mov %s, 0", rdx);
                println("  div %s", rdi);
            } else {
                println("  c%s", id == I64 ? "qo" : "dq");
                println("  idiv %s", rdi);
            }
            if (node->kind == ND_MOD) {
                println("  mov rax, rdx");
            }
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ: {
            bool is_u = node->lhs->type->is_unsigned;
            println("  cmp %s, %s", rax, rdi);
            switch (node->kind) {
                case ND_EQ: println("  sete al"); break;
                case ND_NEQ: println("  setne al"); break;
                case ND_LS: println("  set%c al", is_u ? 'b' : 'l'); break;
                case ND_LEQ: println("  set%ce al", is_u ? 'b' : 'l'); break;
            }
            println("  movzx rax, al");
            return;
        }
        case ND_BITAND: println("  and %s, %s", rax, rdi); return;
        case ND_BITOR: println("  or %s, %s", rax, rdi); return;
        case ND_BITXOR: println("  xor %s, %s", rax, rdi); return;
        case ND_BITSHL:
            println("  mov rcx, rdi");
            println("  shl %s, cl", rax);
            return;
        case ND_BITSHR: {
            bool is_u = node->lhs->type->is_unsigned;
            println("  mov rcx, rdi");
            println("  s%cr %s, cl", is_u ? 'h' : 'a', rax);
            return;
        }
    }
}

static void gen_stmt(Node *node) {
    loc(node);

    switch (node->kind) {
        case ND_EXPR_STMT: gen_expr(node->lhs); return;
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
        case ND_FOR: {
            int i = count();
            if (node->init) {
                gen_stmt(node->init);
            }
            println(".L.begin.%03d:", i);
            if (node->cond) {
                gen_expr(node->cond);
                println("  cmp rax, 0");
                println("  je %s", node->break_label);
            }
            gen_stmt(node->then);
            println("%s:", node->continue_label);
            if (node->upd) {
                gen_expr(node->upd);
            }
            println("  jmp .L.begin.%03d", i);
            println("%s:", node->break_label);
            return;
        }
        case ND_DO: {
            int i = count();
            println(".L.begin.%03d:", i);
            gen_stmt(node->then);
            println("%s:", node->continue_label);
            gen_expr(node->cond);
            println("  cmp rax, 0");
            println("  jne .L.begin.%03d", i);
            println("%s:", node->break_label);
            return;
        }
        case ND_GOTO: println("  jmp %s", node->unique_label); return;
        case ND_LABEL:
            println("%s:", node->unique_label);
            gen_stmt(node->lhs);
            return;
        case ND_SWITCH:
            gen_expr(node->cond);

            for (Node *c = node->case_next; c; c = c->case_next) {
                char *reg = (node->cond->type->size == 8) ? "rax" : "eax";
                println("  cmp %s, %ld", reg, c->val);
                println("  je %s", c->label);
            }

            if (node->default_case) {
                println("  jmp %s", node->default_case->label);
            }

            println("  jmp %s", node->break_label);
            gen_stmt(node->then);
            println("%s:", node->break_label);
            return;
        case ND_CASE:
            println("%s:", node->label);
            gen_stmt(node->lhs);
            return;
        case ND_RETURN:
            if (node->lhs) gen_expr(node->lhs);
            println("  jmp .L.return.%s", obj->name);
            return;
    }

    error_tok(node->tok, "不正な文です");
}

void emit_data(Object *obj) {
    println(".%sal %s", obj->is_static ? "loc" : "glob", obj->name);
    println(".align %d", obj->align);

    if (obj->init_data) {
        println(".data");
        println("%s:", obj->name);

        Relocation *rel = obj->rel;
        int pos = 0;
        while (pos < obj->type->size) {
            if (rel && rel->offset == pos) {
                println("  .quad %s%+ld", rel->label, rel->addend);
                rel = rel->next;
                pos += 8;
            } else {
                println("  .byte %d", obj->init_data[pos++]);
            }
        }
        return;
    }

    println(".bss");
    println("%s:", obj->name);
    println("  .zero %d", (int)obj->type->size);
}

void emit_text(Object *obj) {
    RegAlias64 param_regs[] = {RDI, RSI, RDX, RCX, R8, R9};
    int size = 0;
    for (Object *var = obj->locals; var; var = var->next) {
        size += var->type->size;
        size = align(size, var->align);
        var->offset = -size;
    }
    size = align(size, 16);

    println(".%sal %s", obj->is_static ? "loc" : "glob", obj->name);
    println(".text");
    println("%s:", obj->name);

    println("  push rbp");
    println("  mov rbp, rsp");
    println("  sub rsp, %d", size);

    if (obj->va_area) {
        int gp = 0;
        for (Object *var = obj->params; var; var = var->next) gp++;
        int off = obj->va_area->offset;

        println("  mov DWORD PTR [rbp%+d], %d", off, gp * 8);
        println("  mov DWORD PTR [rbp%+d], 0", off + 4);
        println("  mov QWORD PTR [rbp%+d], rbp", off + 16);
        println("  add QWORD PTR [rbp%+d], %d", off + 16, off + 24);

        println("  mov QWORD PTR [rbp%+d], rdi", off + 24);
        println("  mov QWORD PTR [rbp%+d], rsi", off + 32);
        println("  mov QWORD PTR [rbp%+d], rdx", off + 40);
        println("  mov QWORD PTR [rbp%+d], rcx", off + 48);
        println("  mov QWORD PTR [rbp%+d], r8", off + 56);
        println("  mov QWORD PTR [rbp%+d], r9", off + 64);
        println("  movsd [rbp%+d], xmm0", off + 72);
        println("  movsd [rbp%+d], xmm1", off + 80);
        println("  movsd [rbp%+d], xmm2", off + 88);
        println("  movsd [rbp%+d], xmm3", off + 96);
        println("  movsd [rbp%+d], xmm4", off + 104);
        println("  movsd [rbp%+d], xmm5", off + 112);
        println("  movsd [rbp%+d], xmm6", off + 120);
        println("  movsd [rbp%+d], xmm7", off + 128);
    }

    {
        int i = 0;
        for (Object *p = obj->params; p; p = p->next) {
            TypeId id = type_id(p->type);
            println("  mov %s [rbp%+d], %s", word(id), p->offset,
                    reg(param_regs[i++], id));
        }
    }

    for (Node *s = obj->body; s; s = s->next) gen_stmt(s);

    println(".L.return.%s:", obj->name);
    println("  mov rsp, rbp");
    println("  pop rbp");
    println("  ret");
}

void generate(Object *globals) {
    println(".intel_syntax noprefix");
    for (obj = globals; obj; obj = obj->next) {
        if (!obj->is_def) continue;

        if (obj->is_func) {
            emit_text(obj);
        } else {
            emit_data(obj);
        }
    }
}
