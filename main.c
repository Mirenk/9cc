#include <stdio.h>
#include "9cc.h"

// 入力プログラム
char *user_input;

// 現在着目しているトークン
Token *token;

int main(int argc, char **argv) {
    // 引数の数を確認
    // 1番目はミニキャンでやった通り、通常自分自身がある
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];

    // トークナイズする
    token = tokenize(user_input);
    program();

    codegen();

    return 0;
}