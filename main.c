#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lion.h"

static char *user_input;
static char *filename;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // loc が含まれている行の開始地点と終了地点を取得
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }
    char *end = loc;
    while (*end != '\n') {
        end++;
    }
    // 見つかった行が全体の何行目なのかを調べる
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }
    // 見つかった行を、ファイル名と行番号と一緒に表示
    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = loc - line + indent;
    if (pos > 0) {
        fprintf(stderr, "%*s", pos, " ");  // pos 個の空白を出力
    }
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

char *read_file(char *path) {
    FILE *fp;

    if (strcmp(path, "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(path, "r");
        if (!fp) {
            error("'%s' を開けません: %s", path, strerror(errno));
        }
    }

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

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: 引数の数が正しくありません", argv[0]);
        return 1;
    }
    filename = argv[1];

    tokenize(user_input = read_file(filename));
    generate(program());

    return 0;
}
