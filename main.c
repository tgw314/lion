#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lion.h"

static char *user_input;
static char *input_path;

static bool cliopt_cc1;
static bool cliopt_hash3;
static char *cliopt_o;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void verror_at(int line_no, int len, char *loc, char *fmt, va_list ap) {
    // loc が含まれている行の開始地点と終了地点を取得
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }
    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    // 見つかった行を、ファイル名と行番号と一緒に表示
    int indent = fprintf(stderr, "%s:%d: ", input_path, line_no);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = loc - line + indent;

    for (int i = 0; i < pos; i++) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "^");
    for (int i = 1; i < len; i++) {
        fprintf(stderr, "~");
    }
    fprintf(stderr, " ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    int line_no = 1;
    for (char *p = user_input; p < loc; p++) {
        if (*p == '\n') {
            line_no++;
        }
    }

    va_list ap;
    va_start(ap, fmt);
    verror_at(line_no, 1, loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (tok->kind == TK_EOF) {
        verror_at(tok->line_no, 1, tok->loc - 1, fmt, ap);
    }
    verror_at(tok->line_no, tok->len, tok->loc, fmt, ap);
}

static FILE *open_file(char *path, char *modes) {
    if (!path || !strcmp(path, "-")) {
        return NULL;
    } else {
        FILE *fp = fopen(path, modes);
        if (!fp) {
            error("'%s' を開けません: %s", path, strerror(errno));
        }
        return fp;
    }
}

static char *read_file(FILE *fp) {
    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    // ファイルを読む
    while (true) {
        char buf2[4096];
        int n = fread(buf2, 1, sizeof(buf2), fp);
        if (n == 0) break;
        fwrite(buf2, 1, n, out);
    }

    if (fp != stdin) fclose(fp);

    // 便利のために "\n\0" で終わらせる
    fflush(out);
    if (buflen == 0 || buf[buflen - 1] != '\n') {
        fputc('\n', out);
    }
    fputc('\0', out);
    fclose(out);
    return buf;
}

static void show_help(int status) {
    fprintf(stderr, "lion [ -o <path> ] <file>\n");
    exit(status);
}

static void parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-###")) {
            cliopt_hash3 = true;
            continue;
        }

        if (!strcmp(argv[i], "-cc1")) {
            cliopt_cc1 = true;
            continue;
        }

        if (!strcmp(argv[i], "--help")) show_help(0);

        if (!strcmp(argv[i], "-o")) {
            if (!argv[i + 1]) show_help(1);  // must be argv[argc] == NULL
            cliopt_o = argv[++i];
            continue;
        }

        if (!strncmp(argv[i], "-o", 2)) {
            cliopt_o = argv[i] + 2;
            continue;
        }

        if (argv[i][0] == '-' && argv[i][1]) {
            error("無効なオプション: %s", argv[i]);
        }

        input_path = argv[i];
    }

    if (!input_path) error("入力ファイルがありません");
}

static void run_subprocess(char **argv) {
    if (cliopt_hash3) {
        fprintf(stderr, "%s", argv[0]);
        for (int i = 1; argv[i]; i++) fprintf(stderr, " %s", argv[i]);
        fprintf(stderr, "\n");
    }

    if (fork() == 0) {
        execvp(argv[0], argv);

        fprintf(stderr, "実行失敗: %s: %s", argv[0], strerror(errno));
        _exit(1);
    }

    int status;
    while (wait(&status) > 0);
    if (status != 0) exit(1);
}

static void run_cc1(int argc, char **argv) {
    char **args = calloc(argc + 10, sizeof(char *));
    memcpy(args, argv, argc * sizeof(char *));
    args[argc] = "-cc1";
    run_subprocess(args);
}

static void cc1(void) {
    FILE *in = open_file(input_path, "r");
    if (!in) in = stdin;

    FILE *out = open_file(cliopt_o, "w");
    if (!out) out = stdout;

    user_input = read_file(in);

    tokenize(user_input);
    Object *prog = program();

    fprintf(out, ".file 1 \"%s\"\n", input_path);
    generate(prog, out);
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    if (cliopt_cc1) {
        cc1();
        return 0;
    }

    run_cc1(argc, argv);
    return 0;
}
