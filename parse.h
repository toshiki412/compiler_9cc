#ifndef PARSE_H
#define PARSE_H

#include "tokenize.h"


//抽象構文木の種類
typedef enum {
    ND_ADD,         // +
    ND_SUB,         // -
    ND_MUL,         // *
    ND_DIV,         // /
    ND_EQ,          // ==
    ND_NE,          // !=
    ND_LT,          // <
    ND_LE,          // <=
    ND_ASSIGN,      // =
    ND_LOCAL_VARIABLE,// ローカル変数
    ND_NUM,         // 整数
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_FOR,
    ND_FOR_LEFT,
    ND_FOR_RIGHT,
    ND_WHILE,
    ND_BLOCK,       //{}
    ND_FUNC_CALL,   //関数呼び出し
    ND_FUNC_DEF,    //関数定義
    ND_ADDR,    //アドレス &
    ND_DEREF,    //参照先の値 *
} NodeKind;


//抽象構文木のノードの型
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs;      //左辺
    Node *rhs;      //右辺
    Node **block;   //kindがND_BLOCKのとき使う ブロックに含まれる式を持つベクタ
    char *funcName; //kindがND_FUNCのとき使う
    Node **funcArgs;    //kindがND_FUNC_DEFのとき使う
    int val;        //kindがND_NUMのとき使う
    int offset;     //kindがND_LocalVariableのとき使う
    Type *type;     //kindがND_LocalVariableのとき使う
};

extern Node *code[];

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

void program();
Node *func();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

Node *define_variable();
Node *variable(Token *tok);
Type *get_type(Node *node);

// BNF  ?はオプションの要素で、存在が必須ではない
/*
program    = func*
func       = "int" ident "(" ("int" ident ("," "int" ident)* )? ")" stmt
stmt       = expr ";"
           | "{" stmt* "}"
           | "return" expr ";"
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";"expr? ")" stmt
           | "int" ("*")* ident ";"
           | "{" stmt* "}" 
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = "sizeof" unary
           |"+"? primary
           | "-"? primary
           | "*" primary
           | "&" primary
primary    = num 
             | ident ( "(" expr* ")")? 
             | "(" expr ")"
*/

#endif