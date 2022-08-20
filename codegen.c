#include <stdio.h>
#include "9cc.h"

static int counter = 0;

char *argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
char *argreg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

void gen(Node *node);

void gen_lval(Node *node) {
    if(node->kind == ND_LVAR) {
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", node->offset);
        printf("  push rax\n");
    } else if(node->kind == ND_DEREF) {
        gen(node->lhs);
    } else {
        error("代入の左辺値が変数ではありません");
    }
}

void gen_calc_ptr(Node *node) {
    if(node->type->ty != INT) {
        printf("  pop rax\n");
        if(node->type->ptr_to->ty == INT) {
            printf("  mov rdi, %d\n", 4);
        } else {
            printf("  mov rdi, %d\n", 8);
        }
        printf("  imul rax, rdi\n");
        printf("  push rax\n");
    }
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NUM:
        printf("  push %d\n", node->val);
        return;
        case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");

        if(node->type->ty == INT) {
            printf("  mov eax, [rax]\n");
            printf("  cdqe\n");
        } else {
            printf("  mov rax, [rax]\n");
        }

        printf("  push rax\n");
        return;
        case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");

        if(node->lhs->type->ty == INT) {
            printf("  mov [rax], edi\n");
            printf("  push rdi\n");
        } else {
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
        }
        return;
        case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
        case ND_IF: {
            int if_counter = counter++;
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            if(!node->els) {
                printf("  je .Lend%d\n", if_counter);
                gen(node->then);
            } else {
                printf("  je .Lelse%d\n", if_counter);
                gen(node->then);
                printf("  jmp .Lend%d\n", if_counter);
                printf(".Lelse%d:\n", if_counter);
                gen(node->els);
            }
            printf(".Lend%d:\n", if_counter);
            return;
        }
        case ND_LOOP: {
            int loop_counter = counter++;
            if(node->init) {
                gen(node->init);
            }
            printf(".Lbegin%d:\n", loop_counter);
            if(node->cond) {
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je .Lend%d\n", loop_counter);
            }
            gen(node->then);
            if(node->inc) {
                gen(node->inc);
            }
            printf("  jmp .Lbegin%d\n", loop_counter);
            printf(".Lend%d:\n", loop_counter);
            return;
        }
        case ND_BLOCK:
        for(Node *cur = node->body; cur; cur = cur->next) {
            gen(cur);
        }
        return;
        case ND_CALL: {
            // 引数
            // 先に値を計算しpush
            int argn = 0;
            for(Node *cur = node->arg; cur; cur = cur->next) {
                gen(cur);
                argn++;
            }
            // 順にpopする
            // そうしないと引数が関数呼び出しの場合にうまく行かない
            for(int i = argn - 1; i >= 0; i--) {
                printf("  pop %s\n", argreg64[i]);
            }
            // rbpを保存
            printf("  push rbp\n");
            printf("  mov rbp, rsp\n");
            // rspアライメント
            printf("  and rsp, -16\n");
            // raxクリア
            printf("  xor rax, rax\n");
            // 関数呼出
            printf("  call %s\n", node->funcname);
            // rbp復元
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            // 値をpush
            printf("  push rax\n");
            return;
        }
        case ND_ADDR:
        gen_lval(node->lhs);
        return;
        case ND_DEREF:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    gen(node->lhs);
    gen_calc_ptr(node->rhs);
    gen(node->rhs);
    gen_calc_ptr(node->lhs);

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


void codegen(Func *prog) {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    for(Func *func = prog; func; func = func->next) {
        printf(".globl %s\n", func->name);
        printf("%s:\n", func->name);

        // プロローグ
        // 変数26個分の領域を確保する
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", func->stack_size);

        // 引数をスタック上のローカル変数に入れる
        int argn = 0;
        for(Node *cur = func->args; cur; cur = cur->next) {
            gen_lval(cur);
            printf("  pop rax\n");
            printf("  mov [rax], %s\n", argreg32[argn]); // 引数がintのみなので
            argn++;
        }

        // 先頭の式から順にコード生成
        for(Node *cur = func->code; cur; cur = cur->next) {
            gen(cur);

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
}
