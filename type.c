#include "9cc.h"
#include "type.h"
#include <stdlib.h>

int get_size(Type *type) {
    if(type->ty == INT) {
        return INT_SIZE;
    } else if (type->ty == PTR){
        return PTR_SIZE;
    } else if (type->ty == ARRAY) {
        return type->array_size * get_size(type->ptr_to);
    }
}

void set_type(Node *node) {
    Node *n;
    Type *type = calloc(1, sizeof(Type));
    if(!node || node->type) {
        return;
    }

    set_type(node->lhs);
    set_type(node->rhs);
    set_type(node->cond);
    set_type(node->then);
    set_type(node->els);
    set_type(node->init);
    set_type(node->inc);

    for(n = node->body; n; n = n->next) {
        set_type(n);
    }

    switch(node->kind) {
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
            if(node->rhs->type->ty == PTR || node->rhs->type->ty == ARRAY) {
                node->type = node->rhs->type;
            } else {
                node->type = node->lhs->type;
            }
            return;
        case ND_RETURN:
            node->type = node->lhs->type;
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_CALL: // とりあえず返り値はintと仮定
            type->ty = INT;
            node->type = type;
            return;
        case ND_ADDR:
            type->ty = PTR;
            type->ptr_to = node->lhs->type;
            node->type = type;
            return;
        case ND_DEREF:
            node->type = node->lhs->type->ptr_to;
            return;

    }
}
