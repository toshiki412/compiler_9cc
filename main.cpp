#include "9cc.h"

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    // �g�[�N�i�C�Y���ăp�[�X����
    user_input = argv[1];
    token = tokenize();
    program();

    // �A�Z���u���̑O���������o��
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    currentFunc = 0;
    for(int i = 0; code[i]; i++){
        currentFunc++;
        gen(code[i]);
    }

    return 0;
}