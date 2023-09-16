#include "9cc.h"

//���ݒ��ڂ��Ă���g�[�N��
Token *token;

// ���̓v���O����
char *user_input;

//���[�J���ϐ�
Lvar *locals[100];
int currentFunc = 0;

//�G���[��񍐂��邽�߂̊֐�
//loc�̓G���[�����������ʒu
void error_at(char *loc, const char *fmt, ...){
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

void error(const char *fmt, ...){
    va_list ap; //�ψ����֐����ŉς̈����𑀍삷�邽�߂̃f�[�^�^
    va_start(ap,fmt); //va_list�̏������A�ψ����̃A�N�Z�X���J�n

    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//���̃g�[�N�������҂��Ă���L���̂Ƃ��̓g�[�N������i�߂Đ^��Ԃ�
//����ȊO�͋U��Ԃ�
bool consume(const char *op){
    if( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)){
            return false;
    }
    token = token->next;
    return true;
}

Token *consume_kind(TokenKind kind){
    if(token->kind != kind){
        return NULL;
    }
    Token* tok = token;
    token = token->next;
    return tok;
}

//���̃g�[�N�������҂��Ă���L���̂Ƃ��̓g�[�N������i�߂�
//����ȊO�̓G���[��񍐂���
void expect(const char *op){
    if( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)){
            error_at(token->str, "Expected \"%s\"", op);
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

bool startswith(char *p, const char *q){
    return memcmp(p, q, strlen(q)) == 0;
}

int is_alnum(char c){
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

// �V�����g�[�N�����쐬����cur�Ɍq����
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    //�V����Token�\���̂̃�����(RAM)�𓮓I�Ɋ��蓖�Ă�BToken�\���̂̃o�C�g�T�C�Y�~1�̃o�C�g�����m�ہB
    Token *tok = static_cast<Token*>(calloc(1, sizeof(Token)));

    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

typedef struct ReservedWord ReservedWord;
struct ReservedWord{
    char *word;
    TokenKind kind;
};

ReservedWord reservedWords[] = {
    {"return", TK_RETURN},
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"while", TK_WHILE},
    {"for", TK_FOR},
    {"", TK_EOF},
};


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

        if (startswith(p, "==") || 
            startswith(p, "!=") ||
            startswith(p, "<=") ||
            startswith(p, ">=")){
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                continue;
        }

        if (strchr("+-*/()<>=;{},&", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        bool found = false;
        for(int i = 0; reservedWords[i].kind != TK_EOF; i++){
            char *w = reservedWords[i].word;
            int wordLen = strlen(w);
            TokenKind kind = reservedWords[i].kind;

            if(startswith(p, w) && !is_alnum(p[wordLen])){
                cur = new_token(kind, cur, p, wordLen);
                p += wordLen;
                found = true;
                break;
            }
        }
        
        if(found){
            continue;
        }

        if ('a' <= *p && *p <= 'z'){
            char *c = p;
            while(is_alnum(*c)){
                c++;
            }
            int len = c - p;
            cur = new_token(TK_IDENT, cur, p, len);
            p = c;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10); //�����񂩂�long�^�i�����^�j�ɕϊ�
            cur->len = p - q;
            continue;
        }

        error_at(p, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, p, 0);

    // ex )12 + 31 - 15�̏ꍇ�Atoken�͈ȉ��̂悤�ɍ\������Ă���
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}

LVar *find_lvar(Token *tok){
    for(LVar *var = locals[currentFunc]; var; var = var->next){
        if(var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    return NULL;
}