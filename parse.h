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
    ND_LOCAL_VARIABLE,  // ローカル変数
    ND_GLOBAL_VARIABLE_DEF,  // グローバル変数の定義 int x; のようなもの
    ND_GLOBAL_VARIABLE_USE,  // グローバル変数の使用 x = 1; のようなもの
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
    ND_ADDR,        //アドレス &
    ND_DEREF,       //参照先の値 *
    ND_STRING,      // 文字列
} NodeKind;

typedef struct Type Type;
struct Type {
    // *a[10] は ARRAY -> PTR -> INT のような数珠繋ぎになる
    enum { // 型の種類
        INT,
        PTR,
        ARRAY,
        CHAR,
    } ty;
    struct Type *ptr_to; // tyがPTRのときポインタの指す先の型
    size_t array_size; // tyがARRAYのとき配列のサイズ
};

typedef struct StringToken StringToken;
struct StringToken {
    int index;
    char *value;
    StringToken *next;
};

//抽象構文木のノードの型
typedef struct Node Node;
struct Variable;
struct Node {
    NodeKind kind;
    Node *lhs;      //左辺
    Node *rhs;      //右辺
    Node **block;   //kindがND_BLOCKのとき使う ブロックに含まれる式を持つベクタ
    char *func_name; //kindがND_FUNCのとき使う
    Node **func_args;    //kindがND_FUNC_DEFのとき使う
    int num_value;        //kindがND_NUMのとき使う
    int offset;     //kindがND_Variableのとき使う
    Type *type;     //kindがND_Variableのとき使う
    char *variable_name; //kindがND_GlobalVariable, ND_LOCAL_VARIABLEのとき使う
    int byte_size;   //kindがND_GlobalVariableのとき使う
    StringToken *string; //kindがND_STRINGのとき使う
    Variable *variable;       //kindがND_GLOBAL_VARIABLE_DEFのとき使う
};

typedef struct Variable Variable;
struct Node;
struct Variable {
    Variable *next; // 次の変数かNULL
    char *name; // 変数の名前
    int len; // 名前の長さ
    int offset; // RBPからのオフセット
    Type *type; // 変数の型
    enum {
        LOCAL_VARIABLE,
        GLOBAL_VARIABLE,
    } kind;
    Node *init; // 初期化式
};

typedef struct DefineFuncOrVariable DefineFuncOrVariable;
struct DefineFuncOrVariable {
    Type *type;
    Token *ident;
};

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int num_value);
Node *new_node_string(StringToken *s);

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

Type *get_type(Node *node);
DefineFuncOrVariable *read_define_first_half();
Node *define_variable(DefineFuncOrVariable *def, Variable **varlist);
Node *variable(Token *tok);
Variable *find_varable(Token *tok);

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