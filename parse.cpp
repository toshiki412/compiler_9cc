#include "9cc.h"

//���ݒ��ڂ��Ă���g�[�N��
Token *token;

// ���̓v���O����
char *user_input;

//���[�J���ϐ�
LocalVariable *locals[100];
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

bool is_alnum(char c){
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

// cur�̎��Ɍq����V�����g�[�N�����쐬����
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
    const char *word;
    TokenKind kind;
};

ReservedWord reservedWords[] = {
    {"return", TK_RETURN},
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"while", TK_WHILE},
    {"for", TK_FOR},
    {"int", TK_TYPE},
    {"", TK_EOF},
};


// ���͕�����p���g�[�N�i�C�Y���Ă����Ԃ�
Token *tokenize() {
    char *input_char_pointer = user_input;
    Token head; //�_�~�[�m�[�h
    head.next = NULL;
    Token *cur = &head;

    // ex)12 + 31 - 15

    while (*input_char_pointer) {
        // �󔒕������X�L�b�v
        if (isspace(*input_char_pointer)) {
        input_char_pointer++;
        continue;
        }

        if (startswith(input_char_pointer, "==") || 
            startswith(input_char_pointer, "!=") ||
            startswith(input_char_pointer, "<=") ||
            startswith(input_char_pointer, ">=")){
                cur = new_token(TK_RESERVED, cur, input_char_pointer, 2);
                input_char_pointer += 2;
                continue;
        }

        if (strchr("+-*/()<>=;{},&", *input_char_pointer)) {
            cur = new_token(TK_RESERVED, cur, input_char_pointer++, 1);
            continue;
        }

        bool found = false;
        for(int i = 0; reservedWords[i].kind != TK_EOF; i++){
            const char *word = reservedWords[i].word;
            int wordLen = strlen(word);
            TokenKind kind = reservedWords[i].kind;

            // p���\���w�܂œǂ�ŁA����w�̎��̕������A���t�@�x�b�g�łȂ��ꍇ
            // w=return�̂Ƃ��Areturnx�̂悤�ɂȂ��Ă��Ȃ������m�F
            if(startswith(input_char_pointer, word) && !is_alnum(input_char_pointer[wordLen])){
                cur = new_token(kind, cur, input_char_pointer, wordLen);
                input_char_pointer += wordLen;
                found = true;
                break;
            }
        }
        
        if(found){
            continue;
        }

        if ('a' <= *input_char_pointer && *input_char_pointer <= 'z'){
            char *c = input_char_pointer;
            // ���������̕ϐ�����Ή�
            while(is_alnum(*c)){
                c++;
            }
            int len = c - input_char_pointer;
            cur = new_token(TK_IDENT, cur, input_char_pointer, len);
            input_char_pointer = c;
            continue;
        }

        if (isdigit(*input_char_pointer)) {
            cur = new_token(TK_NUM, cur, input_char_pointer, 0);
            char *p = input_char_pointer;
            cur->val = strtol(input_char_pointer, &input_char_pointer, 10); //�����񂩂�long�^�i�����^�j�ɕϊ�
            cur->len = input_char_pointer - p;
            continue;
        }

        error_at(input_char_pointer, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, input_char_pointer, 0);

    // ex )12 + 31 - 15�̏ꍇ�Atoken�͈ȉ��̂悤�ɍ\������Ă���
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}

// ���[�J���ϐ��𖼑O�Ō�������B������Ȃ������ꍇ��NULL��Ԃ��B
LocalVariable *find_local_variable(Token *tok){
    for(LocalVariable *var = locals[currentFunc]; var; var = var->next){
        if(var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    return NULL;
}