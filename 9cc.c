#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    // 引数の数を確認
    // 1番目はミニキャンでやった通り、通常自分自身がある
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    char *p = argv[1];

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");
    // strtol()はpから数字以外になるまで読み込み、数字以外の文字が出たらpにポインタを格納。
    // 文字列が全て数字だった場合は0を格納。
    // 第三引数は基数を指定
    printf("  mov rax, %ld\n", strtol(p, &p, 10)); 

    while(*p) {
        if(*p == '+') {
            p++; // 演算子分文字の位置を進める
            printf("  add rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        if(*p == '-') {
            p++;
            printf("  sub rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        fprintf(stderr, "予期しない文字です: '%c'\n", *p);
        return 1;
    }

    printf("  ret\n");
    return 0;
}