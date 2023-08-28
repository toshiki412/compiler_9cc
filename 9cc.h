//�w�b�_�t�@�C��

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//tokenize

//�g�[�N���̎��
typedef enum{
TK_RESERVED,    //�L��
TK_NUM,         //�����g�[�N��
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

//���ݒ��ڂ��Ă���g�[�N��
extern Token *token;

// ���̓v���O����
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
    ND_NUM, // ����
} NodeKind;


typedef struct Node Node;


//���ۍ\���؂̃m�[�h�̌^
struct Node{
    NodeKind kind;
    Node *lhs; //����
    Node *rhs; //�E��
    int val;   //kind��ND_NUM�̂Ƃ��g��
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

//�X�^�b�N�}�V��
void gen(Node *node);
