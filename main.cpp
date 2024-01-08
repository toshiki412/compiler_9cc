#include "9cc.h"

int main(int argc, char **argv) {
    // �����̓��̓t�@�C���ǂݍ���
    for (int i = 1; i < argc; i++) {
        filename = argv[i];
        user_input = read_file(filename);

        // �g�[�N�i�C�Y
        Token *t = tokenize();
        if (!token) {
            token = t; // �ŏ��̃t�@�C���̃g�[�N����token�ɃZ�b�g
        } else {
            Token *tt = token;
            while (true) {
                if (tt->next->kind == TK_EOF) {
                    tt->next = t;
                    break;
                }
                tt = tt->next;
            }
        }
    }


    // �p�[�X
    program();

    // �R�[�h����
    // �A�Z���u���̑O���������o��
    printf(".intel_syntax noprefix\n");
    printf(".bss\n"); // �f�[�^�Z�O�����g�̊J�n(�O���[�o���ϐ��̒�`)
    
    // �O���[�o���ϐ��̒�` �����������Ȃ��ꍇ
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_GLOBAL_VARIABLE_DEF && !code[i]->variable->init_value) {
            gen(code[i]);
        }
    }

    printf(".data\n"); // �f�[�^�Z�O�����g�̊J�n(�����񃊃e�����̒�`)
    // �O���[�o���ϐ��̒�` ��������������ꍇ
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_GLOBAL_VARIABLE_DEF && code[i]->variable->init_value) {
            gen(code[i]);
        }
    }

    // �����񃊃e�����̒�`
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