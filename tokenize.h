#ifndef TOKENIZE_H
#define TOKENIZE_H

//トークンの種類
typedef enum {
TK_RESERVED,    //記号 +, -, *, ==, [, など
TK_IDENT,       //識別子 変数名や関数名
TK_NUM,         //整数トークン
TK_RETURN,      //return
TK_IF,          //if
TK_ELSE,        //else
TK_FOR,         //for
TK_WHILE,       //while
TK_TYPE,        //型
TK_SIZEOF,      //sizeof
TK_STRING,      //文字列
TK_STRUCT,      //structs
TK_TYPEDEF,     //typedef
TK_ENUM,        //enum
TK_BREAK,       //break
TK_CONTINUE,    //continue
TK_SWITCH,      //switch
TK_CASE,        //case
TK_DEFAULT,     //default
TK_EOF,         //入力の終わりを表すトークン
} TokenKind;

//トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMのとき、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ 識別子が一文字だけではなくなった(<, <=)
};

char *read_file(char *path);
void error_at(char *loc, const char *fmt);
void error_at2(char *loc, const char *fmt, char *op);
void error(const char *fmt);
void error2(const char *fmt, const char *op);
void error3(const char *fmt, const char *op, const char *err);
bool consume(const char *op);
bool peek_token_str(const char *op);
Token *consume_kind(TokenKind kind);
void expect(const char *op);
int expect_number();
bool at_eof();
bool startswith(char *p, const char *q);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *read_char_literal(Token *cur, char *start);
char get_escape_char(char c);
Token *tokenize();

#endif