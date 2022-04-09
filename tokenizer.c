#include <ctype.h>
#include <stdarg.h> // 可変個引数の処理に必要
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) { // 可変個引数を利用
    va_list ap; // 可変個の引数をリストとして格納
    va_start(ap, fmt); // apを可変個引数の1番目に向ける
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input; // アドレスなので、loc(エラー箇所) >= user_input(入力文字列の先頭)
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");// pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す
bool consume(char *op) {
    if(token->kind != TK_RESERVED ||
       strlen(op) != token->len ||
       memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが期待している型のトークンのときには、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す
bool consume_kind(TokenKind op) {
    if(token->kind != op) {
        return false;
    }
    token = token->next;
    return true;
}

// トークンがTK_IDENTの場合はトークンのポインタを返す
Token *consume_ident() {
    Token *ident_token = token;
    if(token->kind != TK_IDENT) {
        return NULL;
    }
    token = token->next;
    return ident_token;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if(token->kind != TK_RESERVED ||
       strlen(op) != token->len ||
       memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if(token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, 2) == 0;
}

// 変数の一文字目に利用できる文字であれば真
bool is_ident_head(char c) {
    return ('a' <= c && 'z' >= c) || ('A' <= c && 'Z' >= c) || c == '_';
}

// 変数の二文字目以降に利用できる文字であれば真
bool is_ident(char c) {
    return is_ident_head(c) || ('0' <= c && '9' >= c);
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    Token head; // こいつは空で、head->nextから中身あり
    head.next = NULL;
    Token *cur = &head;

    while(*p) {
        // 空白文字をスキップ
        if(isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_ident(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "if", 2) == 0 && !is_ident(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !is_ident(p[4])) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !is_ident(p[5])) {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        if (strncmp(p, "for", 3) == 0 && !is_ident(p[3])) {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }

        if(is_ident_head(*p)) {
            cur = new_token(TK_IDENT, cur, p, 0);
            char *tmp = p++;
            while(is_ident(*p)) {
                p++;
            }
            cur->len = p - tmp;
            continue;
        }

        if(startswith(p, "==") || startswith(p ,"!=") || startswith(p, ">=") || startswith(p, "<=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if(*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == '=' || *p == ';' || *p == '{' || *p == '}') {
            cur = new_token(TK_RESERVED, cur, p++, 1); // 記号が来たら記号を入れたあとにpを進める
            continue;
        }

        if(isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *tmp = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - tmp;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next; // 中身があるやつの先頭を返して上げる
}

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
