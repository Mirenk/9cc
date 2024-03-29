#include <stdio.h>
#include "9cc.h"

static int counter = 0;

char *argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
char *argreg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *argreg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};

void gen(Node *node);

void gen_load_addr(Node *node) {
    if(node->type->ty == ARRAY) {
        return;
    }
    printf("  pop rax\n");

    if(node->type->ty == INT) {
        printf("  mov eax, [rax]\n");
        printf("  cdqe\n");
    } else if(node->type->size == 1) {
        printf("  movsbq rax, [rax]\n");
    }else {
        printf("  mov rax, [rax]\n");
    }

    printf("  push rax\n");
}

void gen_lval(Node *node) {
    if(node->kind == ND_VAR) {
        if(node->var->is_local) {
            printf("  mov rax, rbp\n");
            printf("  sub rax, %d\n", node->var->offset);
            printf("  push rax\n");
        } else {
            printf("  lea rax, %s[rip]\n", node->var->name);
            printf("  push rax\n");
        }
    } else if(node->kind == ND_DEREF) {
        gen(node->lhs);
    } else {
        error("代入の左辺値が変数ではありません");
    }
}

void gen_calc_ptr(Node *node) {
    if(node->type->ty == PTR || node->type->ty == ARRAY) {
        printf("  pop rax\n");
        printf("  mov rdi, %d\n", get_size(node->type->ptr_to));
        printf("  imul rax, rdi\n");
        printf("  push rax\n");
    }
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NUM:
        printf("  push %d\n", node->val);
        return;
        case ND_VAR:
        gen_lval(node);
        gen_load_addr(node);
        return;
        case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");

        if(node->lhs->type->ty == INT) {
            printf("  mov [rax], edi\n");
            printf("  push rdi\n");
        } else if(node->lhs->type->ty == CHAR) {
            printf("  mov [rax], dil\n");
            printf("  push rdi\n");
        }else {
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
        gen_load_addr(node);
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

void datagen(Obj *prog) {
    for(Obj *var = prog; var; var = var->next) {
        if(var->is_func) {
            continue;
        }

        printf("  .data\n");
        printf("  .globl %s\n", var->name);
        printf("%s:\n", var->name);
        if(var->init_data) {
            printf("  .string %s\n", var->init_data);
        } else {
            printf("  .zero %d\n", var->type->size);
        }
    }
}

void textgen(Obj *prog) {
    for(Obj *func = prog; func; func = func->next) {
        if(!func->is_func) {
            continue;
        }

        printf("  .globl %s\n", func->name);
        printf("  .text\n");
        printf("%s:\n", func->name);

        // プロローグ
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", func->stack_size);

        // 引数をスタック上のローカル変数に入れる
        int argn = 0;
        for(Node *cur = func->args; cur; cur = cur->next) {
            gen_lval(cur);
            printf("  pop rax\n");
            if(cur->var->type->size == 1) {
                printf("  mov [rax], %s\n", argreg8[argn]); // 引数がchar
            } else {
                printf("  mov [rax], %s\n", argreg32[argn]); // 引数がintのみなので
            }
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

void codegen(Obj *prog) {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    datagen(prog);
    textgen(prog);
}
