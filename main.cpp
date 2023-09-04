#include "9cc.h"

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    // ex) argv[1] : 12 + 31 - 15

    // �g�[�N�i�C�Y���ăp�[�X����
    user_input = argv[1];
    token = tokenize();
    program();

    // �A�Z���u���̑O���������o��
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //�v�����[�O
    printf(" push rbp\n");
    printf(" mov rbp, rsp\n");
    printf(" sub rsp, 208\n");

    for(int i = 0; code[i]; i++){
        gen(code[i]);
        printf(" pop rax\n");
    }

    //�G�s���[�O
    printf(" mov rsp, rbp\n");
    printf(" pop rbp\n");
    printf(" ret\n");

    return 0;
}