//�w�b�_�t�@�C��
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//tokenize
//�g�[�N���̎��
typedef enum {
TK_RESERVED,    //�L��
TK_IDENT,       //���ʎq
TK_NUM,         //�����g�[�N��
TK_RETURN,      //return
TK_IF,          //if
TK_ELSE,        //else
TK_FOR,         //for
TK_WHILE,       //while
TK_TYPE,        //�^
TK_SIZEOF,      //sizeof
TK_EOF,         //���͂̏I����\���g�[�N��
} TokenKind;

//�g�[�N���^
typedef struct Token Token;
struct Token {
    TokenKind kind; //�g�[�N���̌^
    Token *next;    //���̓��̓g�[�N��
    int val;        //kind��TK_NUM�̂Ƃ��A���̐��l
    char *str;      //�g�[�N��������
    int len;        //�g�[�N���̒��� ���ʎq���ꕶ�������ł͂Ȃ��Ȃ���(<, <=)
};

typedef struct Type Type;
struct Type {
    // *a[10] �� ARRAY -> PTR -> INT �̂悤�Ȑ���q���ɂȂ�
    enum {
        INT,
        PTR,
        ARRAY,
    } ty;
    struct Type *ptr_to;
    size_t array_size;
};

// ���������̃��[�J���ϐ���Ή�
typedef struct LocalVariable LocalVariable;
struct LocalVariable {
    LocalVariable *next; // ���̕ϐ���NULL
    char *name; // �ϐ��̖��O
    int len; // ���O�̒���
    int offset; // RBP����̃I�t�Z�b�g
    Type *type; // �|�C���^
};

//���ݒ��ڂ��Ă���g�[�N��
extern Token *token;

// ���̓v���O����
extern char *user_input;

// ���[�J���ϐ�
// locals��H���ĕϐ��������Ă������ƂŊ����̕ϐ����ǂ������킩��
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
//���ۍ\���؂̎��
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
    ND_LOCAL_VARIABLE,// ���[�J���ϐ�
    ND_NUM,         // ����
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_FOR,
    ND_FOR_LEFT,
    ND_FOR_RIGHT,
    ND_WHILE,
    ND_BLOCK,       //{}
    ND_FUNC_CALL,   //�֐��Ăяo��
    ND_FUNC_DEF,    //�֐���`
    ND_ADDR,    //�A�h���X &
    ND_DEREF,    //�Q�Ɛ�̒l *
} NodeKind;


//���ۍ\���؂̃m�[�h�̌^
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs;      //����
    Node *rhs;      //�E��
    Node **block;   //kind��ND_BLOCK�̂Ƃ��g�� �u���b�N�Ɋ܂܂�鎮�����x�N�^
    char *funcName; //kind��ND_FUNC�̂Ƃ��g��
    Node **funcArgs;    //kind��ND_FUNC_DEF�̂Ƃ��g��
    int val;        //kind��ND_NUM�̂Ƃ��g��
    int offset;     //kind��ND_LocalVariable�̂Ƃ��g��
    Type *type;     //kind��ND_LocalVariable�̂Ƃ��g��
};

extern Node *code[];

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// BNF  ?�̓I�v�V�����̗v�f�ŁA���݂��K�{�ł͂Ȃ�
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
