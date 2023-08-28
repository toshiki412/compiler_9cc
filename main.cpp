#include "9cc.h"

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    // ex) argv[1] : 12 + 31 - 15

    // �g�[�N�i�C�Y���Ăρ[������
    user_input = argv[1];
    token = tokenize();
    Node *node = expr();

    // �A�Z���u���̑O���������o��
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //���ۍ\���؂�����Ȃ���R�[�h����
    gen(node);

    //�X�^�b�N�g�b�v�Ɏ��S�̂̒l�������Ă���͂��Ȃ̂ł����rax�Ƀ��[�h���Ċ֐�����̕Ԃ�l�Ƃ���
    printf(" pop rax\n");
    printf(" ret\n");
    return 0;
}