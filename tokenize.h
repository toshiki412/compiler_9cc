#ifndef TOKENIZE_H
#define TOKENIZE_H

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

//現在着目しているトークン
extern Token *token;

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

#endif