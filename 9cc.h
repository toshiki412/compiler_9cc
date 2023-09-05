//�w�b�_�t�@�C��

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//tokenize

//�g�[�N���̎��
typedef enum{
TK_RESERVED,    //�L��
TK_IDENT,       //���ʎq
TK_NUM,         //�����g�[�N��
TK_RETURN,      //return
TK_EOF,         //���͂̏I����\���g�[�N��
} TokenKind;


typedef struct Token Token;

//�g�[�N���^
struct Token{
    TokenKind kind; //�g�[�N���̌^
    Token *next;    //���̓��̓g�[�N��
    int val;        //kind��TK_NUM�̂Ƃ��A���̐��l
    char *str;      //�g�[�N��������
    int len;        //�g�[�N���̒���
};

typedef struct LVar Lvar;

struct LVar{
    Lvar *next;
    char *name;
    int len;
    int offset;
};

//���ݒ��ڂ��Ă���g�[�N��
extern Token *token;

// ���̓v���O����
extern char *user_input;

extern LVar *locals;

void error_at(char *loc, const char *fmt, ...);
void error(const char *fmt, ...);

bool consume(const char *op);
Token* consume_ident();
Token* consume_return();

void expect(const char *op);

int expect_number();

bool at_eof();
bool startswith(char *p, const char *q);

LVar *find_lvar(Token *tok);

Token *new_token(TokenKind kind, Token *cur, char *str, int len) ;

Token *tokenize();


//codegen

//���ۍ\���؂̎��
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
    ND_LVAR, // ���[�J���ϐ�
    ND_NUM, // ����
    ND_RETURN, // return
} NodeKind;


typedef struct Node Node;


//���ۍ\���؂̃m�[�h�̌^
struct Node{
    NodeKind kind;
    Node *lhs; //����
    Node *rhs; //�E��
    int val;   //kind��ND_NUM�̂Ƃ��g��
    int offset; //kind��ND_LVAR�̂Ƃ��g��
};

extern Node *code[];

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// BNF
// program    = stmt*
// stmt       = expr ";"
//              | return expr ";"
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
