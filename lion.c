#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の数が正しくありません\n");
        return 1;
    }

    printf(
        ".intel_syntax noprefix\n"
        ".globl main\n"
        "main:\n"
        "  mov rax, %d\n"
        "  ret\n",
        atoi(argv[1]));

    return 0;
}
