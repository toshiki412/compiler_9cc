//�w�b�_�t�@�C��
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenize.h"
#include "codegen.h"
#include "parse.h"


// ���̓v���O����
extern char *user_input;

//���ݒ��ڂ��Ă���g�[�N��
extern Token *token;

// ���[�J���ϐ�
// locals��H���ĕϐ��������Ă������ƂŊ����̕ϐ����ǂ������킩��
extern Variable *locals[];

extern Variable *globals[];

extern int current_func;
extern Node *code[];
extern StringToken *strings;