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

typedef enum { I8, I16, I32, I64, U8, U16, U32, U64, F32, F64 } TypeId;

static TypeId type_id(Type *type) {
    // clang-format off
    switch (type->kind) {
        case TY_VOID:   unreachable();
        case TY_BOOL:
        case TY_CHAR:   return type->is_unsigned ? U8 : I8;
        case TY_SHORT:  return type->is_unsigned ? U16 : I16;
        case TY_ENUM:
        case TY_INT:    return type->is_unsigned ? U32 : I32;
        case TY_LONG:   return type->is_unsigned ? U64 : I64;
        case TY_FLOAT:  return F32;
        case TY_DOUBLE: return F64;
        default:        return U64;
    }
    // clang-format on
}

static char *intreg(RegAlias64 reg, TypeId id) {
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

    if (id >= F32) unreachable();
    return regs[reg][id % 4];
}

static char *intword(TypeId id) {
    switch (id) {
        case I8:
        case U8:
            return "BYTE PTR";
        case I16:
        case U16:
            return "WORD PTR";
        case I32:
        case U32:
            return "DWORD PTR";
        case I64:
        case U64:
            return "QWORD PTR";
        default:
            unreachable();
    }
}

static void push(const char *r) {
    println("  push %s", r);
    offset += 8;
}

static void pop(const char *r) {
    println("  pop %s", r);
    offset -= 8;
}

static void pushf(const char *r) {
    println("  sub rsp, 8");
    println("  movsd [rsp], %s", r);
    offset += 8;
}

static void popf(int r) {
    println("  movsd xmm%d, [rsp]", r);
    println("  add rsp, 8");
    offset -= 8;
}

static void push_args(Node *args) {
    if (args) {
        push_args(args->next);

        gen_expr(args);
        if (is_floatnum(args->type)) {
            pushf("xmm0");
        } else {
            push("rax");
        }
    }
}

static void load(Type *type) {
    switch (type->kind) {
        case TY_ARRAY:
        case TY_STRUCT:
        case TY_UNION:
        case TY_FUNC:
            return;
        case TY_FLOAT:
            println("  movss xmm0, [rax]");
            return;
        case TY_DOUBLE:
            println("  movsd xmm0, [rax]");
            return;
        default: {
            TypeId id = type_id(type);

            switch (id) {
                case I8:
                case I16:
                    println("  movsx eax, %s [rax]", intword(id));
                    return;
                case U8:
                case U16:
                    println("  movzx eax, %s [rax]", intword(id));
                    return;
                case U32:
                case I32:
                    println("  movsxd rax, %s [rax]", intword(id));
                    return;
                case U64:
                case I64:
                    println("  mov rax, %s [rax]", intword(id));
                    return;
                default:
                    unreachable();
            }
        }
    }
}

static void cmp_zero(Type *type) {
    switch (type->kind) {
        case TY_FLOAT:
        case TY_DOUBLE: {
            char sfx = (type->kind == TY_FLOAT) ? 's' : 'd';
            println("  xorp%c xmm1, xmm1", sfx);
            println("  ucomis%c xmm0, xmm1", sfx);
            return;
        }
        default:
            if (is_integer(type) && type->size <= 4) {
                println("  cmp eax, 0");
            } else {
                println("  cmp rax, 0");
            }
    }
}

static void cast(Type *from, Type *to) {
    // clang-format off
    static char i32i8[]  = "movsx eax, al";
    static char i32u8[]  = "movzx eax, al";
    static char i32i16[] = "movsx eax, ax";
    static char i32u16[] = "movzx eax, ax";
    static char i32f32[] = "cvtsi2ss xmm0, eax";
    static char i32i64[] = "movsxd rax, eax";
    static char i32f64[] = "cvtsi2sd xmm0, eax";

    static char u32f32[] = "mov eax, eax; cvtsi2ss xmm0, rax";
    static char u32i64[] = "mov eax, eax";
    static char u32f64[] = "mov eax, eax; cvtsi2sd xmm0, rax";

    static char i64f32[] = "cvtsi2ss xmm0, rax";
    static char i64f64[] = "cvtsi2sd xmm0, rax";

    static char u64f32[] = "cvtsi2ss xmm0, rax";
    static char u64f64[] =
        "test rax, rax; js 1f; pxor xmm0, xmm0; cvtsi2sd xmm0, rax; jmp 2f; "
        "1: mov rdi, rax; and eax, 1; pxor xmm0, xmm0; shr rdi; "
        "or rdi, rax; cvtsi2sd xmm0, rdi; addsd xmm0, xmm0; 2:";

    static char f32i8[]  = "cvttss2si eax, xmm0; movsx eax, al";
    static char f32u8[]  = "cvttss2si eax, xmm0; movzx eax, al";
    static char f32i16[] = "cvttss2si eax, xmm0; movsx eax, ax";
    static char f32u16[] = "cvttss2si eax, xmm0; movzx eax, ax";
    static char f32i32[] = "cvttss2si eax, xmm0";
    static char f32u32[] = "cvttss2si rax, xmm0";
    static char f32i64[] = "cvttss2si rax, xmm0";
    static char f32u64[] = "cvttss2si rax, xmm0";
    static char f32f64[] = "cvtss2sd xmm0, xmm0";

    static char f64i8[]  = "cvttsd2si eax, xmm0; movsx eax, al";
    static char f64u8[]  = "cvttsd2si eax, xmm0; movzx eax, al";
    static char f64i16[] = "cvttsd2si eax, xmm0; movsx eax, ax";
    static char f64u16[] = "cvttsd2si eax, xmm0; movzx eax, ax";
    static char f64i32[] = "cvttsd2si eax, xmm0";
    static char f64u32[] = "cvttsd2si rax, xmm0";
    static char f64f32[] = "cvtsd2ss xmm0, xmm0";
    static char f64i64[] = "cvttsd2si rax, xmm0";
    static char f64u64[] = "cvttsd2si rax, xmm0";

    static char *cast_table[][10] = {
        // i8   i16     i32     i64     u8     u16     u32     u64     f32     f64
        {NULL,  NULL,   NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64}, // i8
        {i32i8, NULL,   NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64}, // i16
        {i32i8, i32i16, NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64}, // i32
        {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   i64f32, i64f64}, // i64

        {i32i8, NULL,   NULL,   i32i64, NULL,  NULL,   NULL,   i32i64, i32f32, i32f64}, // u8
        {i32i8, i32i16, NULL,   i32i64, i32u8, NULL,   NULL,   i32i64, i32f32, i32f64}, // u16
        {i32i8, i32i16, NULL,   u32i64, i32u8, i32u16, NULL,   u32i64, u32f32, u32f64}, // u32
        {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   u64f32, u64f64}, // u64

        {f32i8, f32i16, f32i32, f32i64, f32u8, f32u16, f32u32, f32u64, NULL,   f32f64}, // f32
        {f64i8, f64i16, f64i32, f64i64, f64u8, f64u16, f64u32, f64u64, f64f32, NULL},   // f64
    };
    // clang-format on

    if (to->kind == TY_VOID) return;

    if (to->kind == TY_BOOL) {
        cmp_zero(from);
        println("  setne al");
        println("  movzx eax, al");
        return;
    }

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
        case ND_VAR:
            if (node->var->is_local) {
                println("  lea rax, [rbp%+d]", node->var->offset);
                return;
            }

            if (node->type->kind == TY_FUNC && !node->var->is_def) {
                println("  mov rax, QWORD PTR \"%s\"@GOTPCREL[rip]",
                        node->var->name);
                return;
            }

            println("  lea rax, \"%s\"[rip]", node->var->name);
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
        case ND_NULL_EXPR:
            return;
        case ND_NUM: {
            union {
                float f32;
                double f64;
                uint32_t u32;
                uint64_t u64;
            } u;

            switch (node->type->kind) {
                case TY_FLOAT:
                    u.f32 = node->fval;
                    println("  mov eax, %u  # float %f", u.u32, node->fval);
                    println("  movq xmm0, rax");
                    return;
                case TY_DOUBLE:
                    u.f64 = node->fval;
                    println("  mov rax, %lu  # double %f", u.u64, node->fval);
                    println("  movq xmm0, rax");
                    return;
                default:
                    println("  mov rax, %ld", node->val);
                    return;
            }
        }
        case ND_NEG:
            gen_expr(node->lhs);
            switch (node->type->kind) {
                case TY_FLOAT:
                case TY_DOUBLE:
                    println("  mov rax, 1");
                    println("  shl rax, %d", node->type->size * 8 - 1);
                    println("  movq xmm1, rax");
                    println("  xorp%c xmm0, xmm1",
                            (node->type->kind == TY_FLOAT) ? 's' : 'd');
                    return;
                default:
                    println("  neg rax");
                    return;
            }
        case ND_NOT:
            gen_expr(node->lhs);
            cmp_zero(node->lhs->type);
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
            cmp_zero(node->lhs->type);
            println("  je .L.false.%d", c);
            gen_expr(node->rhs);
            cmp_zero(node->rhs->type);
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
            cmp_zero(node->lhs->type);
            println("  jne .L.true.%d", c);
            gen_expr(node->rhs);
            cmp_zero(node->rhs->type);
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
            switch (node->type->kind) {
                case TY_STRUCT:
                case TY_UNION:
                    for (int i = 0; i < node->type->size; i++) {
                        println("  mov r8b, [rax%+d]", i);
                        println("  mov [rdi%+d], r8b", i);
                    }
                    return;
                case TY_FLOAT:
                    println("  movss [rdi], xmm0");
                    return;
                case TY_DOUBLE:
                    println("  movsd [rdi], xmm0");
                    return;
                default: {
                    TypeId id = type_id(node->type);
                    println("  mov %s [rdi], %s", intword(id), intreg(RAX, id));
                    return;
                }
            }
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
            cmp_zero(node->cond->type);
            println("  je .L.else.%03d", c);
            gen_expr(node->then);
            println("  jmp .L.end.%03d", c);
            println(".L.else.%03d:", c);
            gen_expr(node->els);
            println(".L.end.%03d:", c);
            return;
        }
        case ND_ADDR:
            gen_lval(node->lhs);
            return;
        case ND_DEREF:
            gen_expr(node->lhs);
            load(node->type);
            return;
        case ND_CAST:
            gen_expr(node->lhs);
            cast(node->lhs->type, node->type);
            return;
        case ND_CALL: {
            push_args(node->args);
            gen_expr(node->lhs);

            char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            int iargc = 0, fargc = 0;
            for (Node *arg = node->args; arg; arg = arg->next) {
                if (is_floatnum(arg->type)) {
                    popf(fargc++);
                } else {
                    pop(arg_regs[iargc++]);
                }
            }

            if (offset % 16) {
                println("  sub rsp, 8");
                println("  call rax");
                println("  add rsp, 8");
            } else {
                println("  call rax");
            }

            char *sfx = node->type->is_unsigned ? "zx" : "sx";
            switch (node->type->kind) {
                case TY_BOOL:
                    println("  movzx eax, al");
                    return;
                case TY_CHAR:
                    println("  mov%s eax, al", sfx);
                    return;
                case TY_SHORT:
                    println("  mov%s eax, ax", sfx);
                    return;
            }
            return;
        }
        case ND_STMT_EXPR:
            for (Node *n = node->body; n; n = n->next) gen_stmt(n);
            return;
    }

    if (is_floatnum(node->lhs->type)) {
        gen_expr(node->rhs);
        pushf("xmm0");
        gen_expr(node->lhs);
        popf(1);

        char *sfx = (node->lhs->type->kind == TY_FLOAT) ? "ss" : "sd";
        switch (node->kind) {
            case ND_ADD:
                println("  add%s xmm0, xmm1", sfx);
                return;
            case ND_SUB:
                println("  sub%s xmm0, xmm1", sfx);
                return;
            case ND_MUL:
                println("  mul%s xmm0, xmm1", sfx);
                return;
            case ND_DIV:
                println("  div%s xmm0, xmm1", sfx);
                return;
            case ND_EQ:
            case ND_NEQ:
            case ND_LS:
            case ND_LEQ:
                println("  ucomi%s xmm1, xmm0", sfx);
                switch (node->kind) {
                    case ND_EQ:
                        println("  sete al");
                        println("  setnp dl");
                        println("  and al, dl");
                        break;
                    case ND_NEQ:
                        println("  setne al");
                        println("  setp dl");
                        println("  or al, dl");
                        break;
                    case ND_LS:
                        println("  seta al");
                        break;
                    case ND_LEQ:
                        println("  setae al");
                        break;
                }

                println("  and al, 1");
                println("  movzx rax, al");
                return;
        }
        error_tok(node->tok, "誤った式です");
    }

    gen_expr(node->rhs);
    push("rax");
    gen_expr(node->lhs);
    pop("rdi");

    Type *t = node->lhs->type;
    TypeId id = (t->kind == TY_LONG || t->ptr_to) ? I64 : I32;
    char *rax = intreg(RAX, id);
    char *rdi = intreg(RDI, id);
    char *rdx = intreg(RDX, id);

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
        case ND_MOD:
        case ND_DIV:
            if (node->type->is_unsigned) {
                println("  mov %s, 0", rdx);
                println("  div %s", rdi);
            } else {
                println("  c%s", (id == I64) ? "qo" : "dq");
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
            char sfx = node->lhs->type->is_unsigned ? 'b' : 'l';
            println("  cmp %s, %s", rax, rdi);
            switch (node->kind) {
                case ND_EQ:
                    println("  sete al");
                    break;
                case ND_NEQ:
                    println("  setne al");
                    break;
                case ND_LS:
                    println("  set%c al", sfx);
                    break;
                case ND_LEQ:
                    println("  set%ce al", sfx);
                    break;
            }
            println("  movzx rax, al");
            return;
        }
        case ND_BITAND:
            println("  and %s, %s", rax, rdi);
            return;
        case ND_BITOR:
            println("  or %s, %s", rax, rdi);
            return;
        case ND_BITXOR:
            println("  xor %s, %s", rax, rdi);
            return;
        case ND_BITSHL:
            println("  mov rcx, rdi");
            println("  shl %s, cl", rax);
            return;
        case ND_BITSHR: {
            println("  mov rcx, rdi");
            println("  s%cr %s, cl", node->lhs->type->is_unsigned ? 'h' : 'a',
                    rax);
            return;
        }
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
            cmp_zero(node->cond->type);
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
                cmp_zero(node->cond->type);
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
            cmp_zero(node->cond->type);
            println("  jne .L.begin.%03d", i);
            println("%s:", node->break_label);
            return;
        }
        case ND_GOTO:
            println("  jmp %s", node->unique_label);
            return;
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
        int gp = 0, fp = 0;
        for (Object *var = obj->params; var; var = var->next) {
            if (is_floatnum(var->type)) {
                fp++;
            } else {
                gp++;
            }
        }
        int off = obj->va_area->offset;

        println("  mov DWORD PTR [rbp%+d], %d", off, gp * 8);
        println("  mov DWORD PTR [rbp%+d], %d", off + 4, fp * 8 + 48);
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

    int gp = 0, fp = 0;
    for (Object *p = obj->params; p; p = p->next) {
        if (is_floatnum(p->type)) {
            char *sfx = (p->type->kind == TY_FLOAT) ? "ss" : "sd";

            println("  mov%s [rbp%+d], xmm%d", sfx, p->offset, fp++);
        } else {
            RegAlias64 param_regs[] = {RDI, RSI, RDX, RCX, R8, R9};
            TypeId id = type_id(p->type);

            println("  mov %s [rbp%+d], %s", intword(id), p->offset,
                    intreg(param_regs[gp++], id));
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
