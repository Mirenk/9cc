#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node =calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

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
Node *compound_stmt();

Node *lvar(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar(tok);
    if(lvar) { // 既存のローカル変数だった場合、その変数のoffsetをそのまま使用
        node->offset = lvar->offset;
    } else { // 新規変数作成
        LVar *lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = tok->str;
        lvar->len = tok->len;
        lvar->offset = locals->offset + 8;
        locals = lvar;

        node->offset = lvar->offset;
    }

    return node;
}

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
        Node *node;
        if(consume("(")) {
            node = calloc(1, sizeof(Node));
            // 関数の場合
            node->kind = ND_CALL;

            // 関数名を記録
            char *funcname = calloc(1, (sizeof(char) * tok->len) + 1);
            strncpy(funcname, tok->str, tok->len);
            node->funcname = funcname;

            // 引数
            Node *head = calloc(1, sizeof(Node));
            Node *cur = head;
            while(!consume(")")) {
                if(cur != head) {
                    expect(",");
                }
                cur->next = expr();
                cur = cur->next;
            }
            node->arg = head->next;

        } else {
            node = lvar(tok);
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
    if(consume("&")) {
        return new_node(ND_ADDR, unary(), NULL); // 単項-の場合、0-xとする
    }
    if(consume("*")) {
        return new_node(ND_DEREF, unary(), NULL); // 単項-の場合、0-xとする
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

    if(consume_kind(TK_IF)) {
        if(consume("(")) {
            node = calloc(1, sizeof(Node));
            node->kind = ND_IF;
            node->cond = expr();
            expect(")");
            node->then = stmt();
            // else
            if(consume_kind(TK_ELSE)) {
                node->els = stmt();
            }
            // else end
            return node;
        }
    }

    if(consume_kind(TK_WHILE)) {
        if(consume("(")) {
            node = calloc(1, sizeof(Node));
            node->kind = ND_LOOP;
            node->cond = expr();
            expect(")");
            node->then = stmt();
            node->init = NULL;
            node->inc = NULL;
            return node;
        }
    }

    if(consume_kind(TK_FOR)) {
        if(consume("(")) {
            node = calloc(1, sizeof(Node));
            node->kind = ND_LOOP;
            if(!consume(";")) {
                node->init = expr();
                expect(";");
            }
            if(!consume(";")) {
                node->cond = expr();
                expect(";");
            }
            if(!consume(")")) {
                node->inc = expr();
                expect(")");
            }
            node->then = stmt();
            return node;
        }
    }

    if(consume("{")) {
        return compound_stmt();
    }

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

Node *compound_stmt() {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;

    Node *cur = node;

    while(!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    node->body = node->next;
    return node;
}

Func *function_definition() {
    LVar *lvar_head = calloc(1, sizeof(LVar));
    lvar_head->offset = 0; // オフセット初期化
    locals = lvar_head;

    Token *tok = expect_ident();
    expect("(");

    Func *func = calloc(1, sizeof(Func));

    // 関数名を記録
    char *name = calloc(1, (sizeof(char) * tok->len) + 1);
    strncpy(name, tok->str, tok->len);
    func->name = name;

    // 引数
    Node *arg_head = calloc(1, sizeof(Node));
    Node *cur = arg_head;
    while(!consume(")")) {
        if(cur != arg_head) {
            expect(",");
        }
        tok = expect_ident();
        if(find_lvar(tok)) {
            error_at(tok->str, "既に宣言されている変数です");
        }
        cur->next = lvar(tok);
        cur = cur->next;
    }
    func->args = arg_head->next;

    // 関数内部のブロック
    expect("{");
    Node *node = compound_stmt();

    func->code = node;
    func->locals = locals;
    func->stack_size = locals->offset;

    return func;
}

Func *program() {
    Func *func_head = calloc(1, sizeof(Func)); // 構文木の先頭を保存しておく配列
    Func *func = func_head;

    while(!at_eof()) {
        func->next = function_definition(); // トークンが終わるまで木を作成し，入れていく
        func = func->next;
    }
    func->next = NULL; // 最後の木の後にnullを入れ，末尾が分かるようにする

    return func_head->next;
}
