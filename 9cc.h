//ヘッダファイル

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//tokenize

//トークンの種類
typedef enum{
TK_RESERVED,    //記号
TK_NUM,         //整数トークン
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

//現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

void error_at(char *loc, char *fmt, ...);

bool consume(char *op);

void expect(char *op);

int expect_number();

bool at_eof();
bool startswith(char *p, char *q);

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
    ND_NUM, // 整数
} NodeKind;


typedef struct Node Node;


//抽象構文木のノードの型
struct Node{
    NodeKind kind;
    Node *lhs; //左辺
    Node *rhs; //右辺
    int val;   //kindがND_NUMのとき使う
};

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// BNF
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

//スタックマシン
void gen(Node *node);
