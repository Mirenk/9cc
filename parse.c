#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"
#include "type.h"

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node =calloc(1, sizeof(Node));
    Type *type = calloc(1, sizeof(Type));
    node->kind = ND_NUM;
    node->val = val;
    type->ty = INT;
    node->type = type;
    return node;
}

Obj *locals; // ローカル変数
Obj *globals;

Obj *find_var(Token *tok) {
    for(Obj *var = locals; var; var = var->next) {
        if(var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }
    for(Obj *var = globals; var; var = var->next) {
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

Type *declarator() {
    Type *type = calloc(1, sizeof(Type));

    if(consume_kind(TK_INT)) {
        type->ty = INT;
    } else if(consume_kind(TK_CHAR)) {
        type->ty = CHAR;
    } else {
        return NULL;
    }

    return type;
}

Type *array(Type *type) {
    if(!consume("[")) {
        return type;
    }

    Type *arr = calloc(1, sizeof(Type));

    arr->ty = ARRAY;
    arr->array_size = expect_number();
    expect("]");
    arr->ptr_to = array(type);
    return arr;
}

Type *pointer(Type *type) {
    Type *prev = type;
    Type *head = prev;

    while(consume("*")) {
        head = calloc(1, sizeof(Type));
        head->ty = PTR;
        head->ptr_to = prev;
        prev = head;
    }

    return head;
}

Obj *new_obj(Type *type) {
    Obj *obj = calloc(1, sizeof(Obj));

    Type *base = pointer(type);
    obj->type = base;
    Token *tok = expect_ident();

    if(find_var(tok)) {
        error("既に定義されているシンボルです。");
    }

    char *name = calloc(1, (sizeof(char) * tok->len) + 1);
    strncpy(name, tok->str, tok->len);
    obj->name = name;
    obj->len = tok->len;

    obj->type->size = get_size(obj->type);

    return obj;
}

Obj *new_gobj(Type *type) {
    Obj *head = new_obj(type);
    head->next = globals;
    globals = head;

    return head;
}

Obj *new_gvar(Obj *gvar) {
    Type *base = gvar->type;

    gvar->type = array(base);
    gvar->is_local = false;
    gvar->type->size = get_size(gvar->type);

    return gvar;

}

char *new_unique_name() {
    static int id = 0;
    char *buf = calloc(1, 20); // 20まで
    sprintf(buf, ".LC%d", id++);
    return buf;
}

Obj *new_string_literal(char *name) {
    Obj *obj = calloc(1, sizeof(Obj));

    obj->init_data = name;

    obj->type = calloc(1, sizeof(Type));
    obj->type->ty = ARRAY;
    obj->type->array_size = strlen(name) - 1;
    obj->type->ptr_to = calloc(1, sizeof(Type));
    obj->type->ptr_to->ty = CHAR;
    obj->type->ptr_to->size = CHAR_SIZE;

    obj->name = new_unique_name();
    obj->len = strlen(obj->name);

    obj->type->size = get_size(obj->type);

    obj->next = globals->next;
    globals->next = obj;

    return obj;
}

Node *new_lvar(Type *type) {
    Node *node = calloc(1, sizeof(Node));
    Obj *lvar = new_obj(type);
    lvar->is_local = true;
    Type *base = lvar->type;
    node->kind = ND_VAR;
    node->var = lvar;

    lvar->type = array(base);

    node->type = lvar->type;

    lvar->next = locals;
    lvar->offset = locals->offset + get_size(lvar->type);
    locals = lvar;

    return node;
}

Node *lvar(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VAR;

    Obj *lvar = find_var(tok);
    if(!lvar) { // 未定義なシンボル
        error("定義されていない変数です。");
    }
    node->var = lvar;
    node->type = lvar->type;

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

    // 文字列リテラル
    tok = consume_kind(TK_STR);
    if(tok) {
        Node *node = calloc(1, sizeof(Node));
        char *literal = calloc(1, (sizeof(char) * tok->len) + 1);
        strncpy(literal, tok->str, tok->len+1);

        Obj *obj = new_string_literal(literal);
        node->var = obj;
        tok = tok->next;

        node->kind = ND_VAR;
        node->type = obj->type;

        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

Node *postfix() {
    Node *node = primary();

    while(consume("[")) {
        Node *index = expr();
        expect("]");
        node = new_node(ND_DEREF, new_node(ND_ADD, node, index), NULL);
    }

    return node;
}

Node *unary() {
    Node *node;
    if(consume("+")) {
        return unary(); // 単項+の場合、+だけ進めてprimaryを呼ぶ
    }
    if(consume("-")) {
        return new_node(ND_SUB, new_node_num(0), unary()); // 単項-の場合、0-xとする
    }
    if(consume("&")) {
        return new_node(ND_ADDR, unary(), NULL); // アドレス演算
    }
    if(consume("*")) {
        return new_node(ND_DEREF, unary(), NULL); // デリファレンス
    }
    if(consume_kind(TK_SIZEOF)) {
        node = unary();
        set_type(node);
        return new_node_num(get_size(node->type));
    }
    return postfix();
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

Node *declaration() {
    Type *type = declarator();

    if(!type) {
        return NULL;
    }

    return new_lvar(type);
}

Node *compound_stmt() {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;

    Node *cur = node;

    while(!consume("}")) {
        cur->next = declaration();
        if(cur->next) {
            expect(";");
        } else {
            cur->next = stmt();
        }
        cur = cur->next;
    }

    node->body = node->next;
    return node;
}

Obj *function_definition(Obj *func) {
    Obj *lvar_head = calloc(1, sizeof(Obj));
    lvar_head->offset = 0; // オフセット初期化
    locals = lvar_head;

    if(!consume("(")) {
        return NULL;
    };
    func->is_func = true;

    // 引数
    Node *arg_head = calloc(1, sizeof(Node));
    Node *cur = arg_head;
    while(!consume(")")) {
        if(cur != arg_head) {
            expect(",");
        }
        cur->next = declaration();
        if(!cur->next) {
            error("型名ではありません。");
        }
        cur = cur->next;
    }
    func->args = arg_head->next;

    // 関数内部のブロック
    expect("{");
    Node *node = compound_stmt();
    set_type(node);

    func->code = node;
    func->locals = locals;
    func->stack_size = locals->offset;

    return func;
}

Obj *program() {
    globals = NULL;

    while(!at_eof()) {
        Obj *obj = new_gobj(declarator());
        Obj *func = function_definition(obj);
        if(!func) {
            // グローバル変数
            new_gvar(obj);
            expect(";");
        }
    }

    return globals;
}
