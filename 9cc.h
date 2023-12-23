//ヘッダファイル
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//tokenize
//トークンの種類
typedef enum {
TK_RESERVED,    //記号
TK_IDENT,       //識別子
TK_NUM,         //整数トークン
TK_RETURN,      //return
TK_IF,          //if
TK_ELSE,        //else
TK_FOR,         //for
TK_WHILE,       //while
TK_TYPE,        //型
TK_SIZEOF,      //sizeof
TK_EOF,         //入力の終わりを表すトークン
} TokenKind;

//トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMのとき、その数値
    char *str;      //トークン文字列
    int len;        //トークンの長さ 識別子が一文字だけではなくなった(<, <=)
};

typedef struct Type Type;
struct Type {
    // *a[10] は ARRAY -> PTR -> INT のような数珠繋ぎになる
    enum {
        INT,
        PTR,
        ARRAY,
    } ty;
    struct Type *ptr_to;
    size_t array_size;
};

// 複数文字のローカル変数を対応
typedef struct LocalVariable LocalVariable;
struct LocalVariable {
    LocalVariable *next; // 次の変数かNULL
    char *name; // 変数の名前
    int len; // 名前の長さ
    int offset; // RBPからのオフセット
    Type *type; // ポインタ
};

//現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

// ローカル変数
// localsを辿って変数名を見ていくことで既存の変数かどうかがわかる
extern LocalVariable *locals[];
extern int currentFunc;

void error_at(char *loc, const char *fmt, ...);
void error(const char *fmt, ...);

bool consume(const char *op);
Token *consume_kind(TokenKind kind);

void expect(const char *op);

int expect_number();

bool at_eof();
bool startswith(char *p, const char *q);

LocalVariable *find_local_variable(Token *tok);

Token *new_token(TokenKind kind, Token *cur, char *str, int len) ;

Token *tokenize();


//codegen
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
void gen_left_value(Node *node);
void gen(Node *node);
