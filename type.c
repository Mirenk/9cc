#include "9cc.h"
#include "type.h"
#include <stdlib.h>

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
