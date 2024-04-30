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

int align(int n, int align) {
    if (align == 0) return n;
    return (n + align - 1) / align * align;
}

typedef enum { I8, I16, I32, I64 } TypeId;

static TypeId type_id(Type *type) {
    switch (type->kind) {
        case TY_BOOL:
        case TY_CHAR: return I8;
        case TY_SHORT: return I16;
        case TY_INT: return I32;
        case TY_VOID: unreachable();
        default: return I64;
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

    return regs[reg][id];
}

static char *word(TypeId id) {
    switch (id) {
        case I8: return "BYTE PTR";
        case I16: return "WORD PTR";
        case I32: return "DWORD PTR";
        case I64: return "QWORD PTR";
        default: unreachable();
    }
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
            println("  movsx %s, %s [%s]", reg(RAX, I32), word(id),
                    reg(RAX, I64));
            return;
        case I32:
            println("  movsxd %s, %s [%s]", reg(RAX, I64), word(id),
                    reg(RAX, I64));
            return;
        case I64:
            println("  mov %s, %s [%s]", reg(RAX, I64), word(id),
                    reg(RAX, I64));
            return;
    }
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
            println("  push rax");
            gen_expr(node->rhs);
            println("  pop rdi");
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
    char *rax = reg(RAX, id);
    char *rdi = reg(RDI, id);

    switch (node->kind) {
        case ND_ADD: println("  add %s, %s", rax, rdi); return;
        case ND_SUB: println("  sub %s, %s", rax, rdi); return;
        case ND_MUL: println("  imul %s, %s", rax, rdi); return;
        case ND_MOD:
        case ND_DIV:
            if (id == I64) {
                println("  cqo");
            } else {
                println("  cdq");
            }
            println("  idiv %s", rdi);
            if (node->kind == ND_MOD) {
                println("  mov rax, rdx");
            }
            return;
        case ND_EQ:
        case ND_NEQ:
        case ND_LS:
        case ND_LEQ:
            println("  cmp %s, %s", rax, rdi);
            switch (node->kind) {
                case ND_EQ: println("  sete al"); break;
                case ND_NEQ: println("  setne al"); break;
                case ND_LS: println("  setl al"); break;
                case ND_LEQ: println("  setle al"); break;
            }
            println("  movzx rax, al");
            return;
        case ND_BITAND: println("  and %s, %s", rax, rdi); return;
        case ND_BITOR: println("  or %s, %s", rax, rdi); return;
        case ND_BITXOR: println("  xor %s, %s", rax, rdi); return;
        case ND_BITSHL:
            println("  mov rcx, rdi");
            println("  shl %s, cl", rax);
            return;
        case ND_BITSHR:
            println("  mov rcx, rdi");
            println("  sar %s, cl", rax);
            return;
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
            gen_expr(node->lhs);
            println("  jmp .L.return.%s", obj->name);
            return;
    }

    error_tok(node->tok, "不正な文です");
}

void emit_data(Object *obj) {
    println(".data");
    if (obj->is_static || obj->name[0] == '.') {
        println(".local %s", obj->name);
    } else {
        println(".globl %s", obj->name);
    }
    println("%s:", obj->name);

    if (obj->init_data) {
        if (!is_integer(obj->type)) {
            for (int i = 0; i < obj->type->array_size; i++) {
                println("  .byte %d", obj->init_data[i]);
            }
        }
    } else {
        println("  .zero %d", (int)obj->type->size);
    }
}

void emit_text(Object *obj) {
    RegAlias64 param_regs[] = {RDI, RSI, RDX, RCX, R8, R9};
    // ローカル変数のオフセットを計算
    int size = 0;
    for (Object *v = obj->locals; v; v = v->next) {
        size += v->type->size;
        size = align(size, v->type->align);
        v->offset = -size;
    }
    obj->stack_size = align(size, 16);

    println(".text");
    if (obj->is_static) {
        println(".local %s", obj->name);
    } else {
        println(".globl %s", obj->name);
    }
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
            TypeId id = type_id(p->type);
            println("  mov %s [rbp%+d], %s", word(id), p->offset,
                    reg(param_regs[i++], id));
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

void generate(Object *globals) {
    println(".intel_syntax noprefix");
    for (obj = globals; obj; obj = obj->next) {
        if (obj->is_func) {
            if (!obj->is_def) continue;
            emit_text(obj);
        } else {
            emit_data(obj);
        }
    }
}
