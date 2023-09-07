//ヘッダファイル

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//tokenize

//トークンの種類
typedef enum{
TK_RESERVED,    //記号
TK_IDENT,       //識別子
TK_NUM,         //整数トークン
TK_RETURN,      //return
TK_IF,          //if
TK_ELSE,        //else
TK_FOR,         //for
TK_WHILE,       //while
TK_EOF,         //入力の終わりを表すトークン
} TokenKind;


typedef struct Token Token;

//トークン型
struct Token{
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMのとき、その数値
    char *str;      //トークン文字列
    int len;        //トークンの長さ
};

typedef struct LVar Lvar;

struct LVar{
    Lvar *next;
    char *name;
    int len;
    int offset;
};

//現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

extern LVar *locals;

void error_at(char *loc, const char *fmt, ...);
void error(const char *fmt, ...);

bool consume(const char *op);
Token *consume_kind(TokenKind kind);

void expect(const char *op);

int expect_number();

bool at_eof();
bool startswith(char *p, const char *q);

LVar *find_lvar(Token *tok);

Token *new_token(TokenKind kind, Token *cur, char *str, int len) ;

Token *tokenize();


//codegen

//抽象構文木の種類
typedef enum{
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_ASSIGN,  // =
    ND_LVAR, // ローカル変数
    ND_NUM, // 整数
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_FOR,
    ND_WHILE,
} NodeKind;


typedef struct Node Node;


//抽象構文木のノードの型
struct Node{
    NodeKind kind;
    Node *lhs; //左辺
    Node *rhs; //右辺
    int val;   //kindがND_NUMのとき使う
    int offset; //kindがND_LVARのとき使う
};

extern Node *code[];

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// BNF  ?はオプションの要素で、存在が必須ではない
// program    = stmt*
// stmt       = expr ";"
//              | "return" expr ";"
//              | "if" "(" expr ")" stmt ("else" stmt)?
//              | "while" "(" expr ")" stmt
//              | "for" "(" expr? ";" expr? ";"expr? ")" stmt
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | ident | "(" expr ")"

void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

void gen_lval(Node *node);
void gen(Node *node);
