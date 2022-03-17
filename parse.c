#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

LVar *locals; // ローカル変数

LVar *find_lvar(Token *tok) {
    for(LVar *var = locals; var; var = var->next) {
        if(var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }
    return NULL;
}

Node *mul();
Node *primary();
Node *unary();
Node *add();
Node *equality();
Node *relational();
Node *expr();
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

Node *primary() {
    // 次のトークンが"("なら、"(" expr ")"のはず
    if(consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // 次のトークンが変数の場合
    Token *tok = consume_ident();
    if(tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if(lvar) { // 既存のローカル変数だった場合、その変数のoffsetをそのまま使用
            node->offset = lvar->offset;
        } else { // 新規変数作成
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->offset = locals->offset + 8;
            node->offset = lvar->offset;
            locals = lvar;
        }
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

Node *assign() {
    Node *node = equality();
    if(consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

Node *expr() {
    return assign();
}

Node *stmt() {
    Node *node;

    if(consume_kind(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else {
        node = expr();
    }
    expect(";");
    return node;
}

Func *program() {
    Node node_head; // 構文木の先頭を保存しておく配列
    Node *code = &node_head;
    LVar lvar_head;
    locals = &lvar_head;
    int i = 0;

    while(!at_eof()) {
        code->next = stmt(); // トークンが終わるまで木を作成し，入れていく
        code = code->next;
    }
    code->next = NULL; // 最後の木の後にnullを入れ，末尾が分かるようにする

    Func *prog = calloc(1, sizeof(Func));
    prog->code = node_head.next;
    prog->locals = locals;
    prog->stack_size = locals->offset;

    return prog;
}