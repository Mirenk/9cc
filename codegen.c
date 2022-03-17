#include <stdio.h>
#include "9cc.h"

void gen_lval(Node *node) {
    if(node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NUM:
        printf("  push %d\n", node->val);
        return;
        case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
        case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch(node->kind) {
        case ND_ADD:
        printf("  add rax, rdi\n");
        break;
        case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
        case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
        case ND_DIV:
        printf("  cqo\n"); //128bitに拡張
        printf("  idiv rdi\n");
        break;
        case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n"); // rax下位8ビットにcmpで同値だったら1，異なったら0をセット
        printf("  movzb rax, al\n"); // alをraxに拡張(上位56ビットをゼロクリア)
        break;
        case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n"); // rax下位8ビットにcmpで同値だったら1，異なったら0をセット
        printf("  movzb rax, al\n"); // alをraxに拡張(上位56ビットをゼロクリア)
        break;
        case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n"); // rax下位8ビットにcmpで同値だったら1，異なったら0をセット
        printf("  movzb rax, al\n"); // alをraxに拡張(上位56ビットをゼロクリア)
        break;
        case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n"); // rax下位8ビットにcmpで同値だったら1，異なったら0をセット
        printf("  movzb rax, al\n"); // alをraxに拡張(上位56ビットをゼロクリア)
        break;
    }

    printf("  push rax\n");
}

void codegen() {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");

    // 先頭の式から順にコード生成
    for(int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っているはずなので，
        // スタックが溢れないようにpopしておく
        printf("  pop rax\n");
    }

    // エピローグ
    // 最後の式の結果がraxに残っているのでそれが返り値になる
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}
