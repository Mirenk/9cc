#include <stdio.h>
#include "9cc.h"

Node *mul();
Node *primary();
Node *unary();
Node *add();
Node *equality();
Node *relational();

Node *primary() {
    // 次のトークンが"("なら、"(" expr ")"のはず
    if(consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

Node *unary() {
    if(consume("+")) {
        return primary(); // 単項+の場合、+だけ進めてprimaryを呼ぶ
    }
    if(consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary()); // 単項-の場合、0-xとする
    }
    return primary(); // その他の場合、今までと同じ
}

Node *mul() {
    Node *node = unary();

    for(;;) {
        if(consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if(consume("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

Node *add() {
    Node *node = mul();

    for(;;) {
        if(consume("+")){
            node = new_node(ND_ADD, node, mul());
        } else if(consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

Node *relational() {
    Node *node = add();

    for(;;) {
        if(consume("<")) {
            node = new_node(ND_LT, node, add());
        } else if(consume(">")) {
            node = new_node(ND_LT, add(), node); // 項を逆にして対応
        } else if(consume("<=")) {
            node = new_node(ND_LE, node, add());
        } else if(consume(">=")) {
            node = new_node(ND_LE, add(), node); // これも逆にする
        } else {
            return node;
        }
    }
}

Node *equality() {
    Node *node = relational();

    for(;;) {
        if(consume("==")) {
            node = new_node(ND_EQ, node, relational());
        } else if(consume("!=")) {
            node = new_node(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

Node *expr() {
    return equality();
}

void gen(Node *node) {
    if(node->kind == ND_NUM) {
        printf("  push %d\n", node->val);
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
