#include <stdbool.h>

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
    TK_RETURN,   // returnを表すトークン
    TK_IF,       // if
    TK_ELSE,     // else
    TK_WHILE,    // while
    TK_FOR,      // for
    TK_INT,      // int
    TK_SIZEOF,   // sizeof
} TokenKind;

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
    ND_ASSIGN, // =
    ND_LVAR,   // ローカル変数
    ND_NUM, // 整数
    ND_RETURN, // return
    ND_IF,  // if
    ND_ELSE, // else
    ND_LOOP, // for | while
    ND_BLOCK, // ブロック
    ND_CALL, // 関数呼出
    ND_ADDR, // 単項&
    ND_DEREF, // 単項*
} NodeKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; //トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

typedef struct Type Type;

// ローカル変数型の型
struct Type {
    enum { INT, PTR } ty;
    struct Type *ptr_to;
};

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // 次のノード
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    Node *cond;    // ifの条件式
    Node *then;    // 条件式が真だった場合のノード
    Node *els;     // elseのノード
    Node *init;    // forの初期化式
    Node *inc;     // forの反復式
    Node *body;    // ブロックの中身ノード
    Node *arg;     // 引数
    Type *type;      // ノードの型
    char *funcname;// kindがND_CALLの場合のみ使う
    int val;       // kindがND_NUMの場合のみ使う
    int offset;    // kindがND_LVARの場合のみ使う
};

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
    LVar *next; // 次の変数かNULL
    Type *type; // 変数の型
    char *name; // 変数名
    int len;    // 変数名の長さ
    int offset; // RBPからのオフセット
};

typedef struct Func Func;

// 関数型
struct Func {
    Func *next;
    char *name;
    Node *code;
    Node *args;
    LVar *locals;
    int stack_size;
};

// 入力プログラム
extern char *user_input;

// 現在着目しているトークン
extern Token *token;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
bool consume_kind(TokenKind op);
Token *consume_ident();
void expect(char *op);
int expect_number();
Token *expect_ident();

bool at_eof();

Token *tokenize(char *p);
Func *program();
void codegen(Func *prog);

void set_type(Node *node);
