#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
};

//���ݒ��ڂ��Ă���g�[�N��
Token *token;

// ���̓v���O����
char *user_input;

//�G���[��񍐂��邽�߂̊֐�
//loc�̓G���[�����������ʒu
void error_at(char *loc, char *fmt, ...){
    va_list ap; //�ψ����֐����ŉς̈����𑀍삷�邽�߂̃f�[�^�^
    va_start(ap,fmt); //va_list�̏������A�ψ����̃A�N�Z�X���J�n

    //pos��loc��user_input����ǂꂾ������Ă��邩��\���I�t�Z�b�g
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
    //printf�̏o�͐�͕W���o�́Afprintf�̏o�͐�͎w�肵���t�@�C���X�g���[���Avfprintf�͉ψ�����^���Ă���
}

//���̃g�[�N�������҂��Ă���L���̂Ƃ��̓g�[�N������i�߂Đ^��Ԃ�
//����ȊO�͋U��Ԃ�
bool consume(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        return false;
    }
    token = token->next;
    return true;
}

//���̃g�[�N�������҂��Ă���L���̂Ƃ��̓g�[�N������i�߂�
//����ȊO�̓G���[��񍐂���
void expect(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        error_at(token->str, "Not '%c'",op);
    }
    token = token->next;
}

//���̃g�[�N�������l�̂Ƃ��A�g�[�N����i�߂Ă��̐��l��Ԃ�
//����ȊO�̓G���[��񍐂���
int expect_number(){
    if(token->kind != TK_NUM){
        error_at(token->str,"Not a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(){
    return token->kind == TK_EOF;
}

// �V�����g�[�N�����쐬����cur�Ɍq����
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = static_cast<Token*>(calloc(1, sizeof(Token)));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

// ���͕�����p���g�[�N�i�C�Y���Ă����Ԃ�
Token *tokenize() {
    char *p = user_input;
    Token head; //�_�~�[�m�[�h
    head.next = NULL;
    Token *cur = &head;

    // ex)12 + 31 - 15

    while (*p) {
        // �󔒕������X�L�b�v
        if (isspace(*p)) {
        p++;
        continue;
        }

        if (*p == '+' || *p == '-') {
        cur = new_token(TK_RESERVED, cur, p++);
        continue;
        }

        if (isdigit(*p)) {
        cur = new_token(TK_NUM, cur, p);
        cur->val = strtol(p, &p, 10); //�����񂩂�long�^�i�����^�j�ɕϊ�
        continue;
        }

        error_at(p, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, p);

    // ex )12 + 31 - 15�̏ꍇ�Atoken�͈ȉ��̂悤�ɍ\������Ă���
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    // �g�[�N�i�C�Y����
    // ex) argv[1] : 12 + 31 - 15
    user_input = argv[1];
    token = tokenize();

    // �A�Z���u���̑O���������o��
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // ���̍ŏ��͐��łȂ���΂Ȃ�Ȃ��̂ŁA������`�F�b�N����
    // �ŏ���mov���߂��o��
    printf("  mov rax, %d\n", expect_number());

    // `+ <��>`���邢��`- <��>`�Ƃ����g�[�N���̕��т������
    // �A�Z���u�����o��
    while (!at_eof()) {
        if (consume('+')) {
        printf("  add rax, %d\n", expect_number());
        continue;
        }

        expect('-');
        printf("  sub rax, %d\n", expect_number());
    }

    printf("  ret\n");
    return 0;
}