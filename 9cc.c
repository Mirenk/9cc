#include <ctype.h>
#include <stdarg.h> // 可変個引数の処理に必要
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; //トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ使う
};


// 入力プログラム
char *user_input;

// 現在着目しているトークン
Token *token;

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

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, 2) == 0;
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

        if(startswith(p, "==") || startswith(p ,"!=") || startswith(p, ">=") || startswith(p, "<=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if(*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>') {
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

Node *expr();
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
    Node *node = expr();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成;
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの返り値
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}