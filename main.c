#include <stdio.h>
#include "9cc.h"

// 入力プログラム
char *user_input;
char *filename;

// 現在着目しているトークン
Token *token;

int main(int argc, char **argv) {
    // 引数の数を確認
    // 1番目はミニキャンでやった通り、通常自分自身がある
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    filename = argv[1];

    user_input = read_file(filename);

    // トークナイズする
    token = tokenize(user_input);
    Obj *prog = program();

    codegen(prog);

    return 0;
}
