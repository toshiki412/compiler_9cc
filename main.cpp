#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    user_input = argv[1];

    // �g�[�N�i�C�Y
    token = tokenize();

    // �p�[�X
    program();

    // �R�[�h����
    // �A�Z���u���̑O���������o��
    printf(".intel_syntax noprefix\n");
    printf(".bss\n"); // �f�[�^�Z�O�����g�̊J�n(�O���[�o���ϐ��̒�`)
    
    // �O���[�o���ϐ��̒�`
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_GLOBAL_VARIABLE_DEF) {
            gen(code[i]);
        }
    }

    printf(".data\n"); // �f�[�^�Z�O�����g�̊J�n(�����񃊃e�����̒�`)
    for (StringToken *s = strings; s; s = s->next) {
        printf(".LC_%d:\n", s->index);
        printf("    .string \"%s\"\n", s->value);
    }

    printf(".text\n"); // �v���O�����̎��s�����̊J�n
    current_func = 0;
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_FUNC_DEF) {
            current_func++;
            gen(code[i]);
        }
    }

    return 0;
}