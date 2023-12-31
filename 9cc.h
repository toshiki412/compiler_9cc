//�w�b�_�t�@�C��
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "tokenize.h"
#include "codegen.h"
#include "parse.h"


// �O���[�o���ϐ�
extern char *user_input;        // ���̓v���O����
extern char *filename;          // ���̓t�@�C����
extern Token *token;            // ���ݒ��ڂ��Ă���g�[�N��
extern Variable *locals[];      // ���[�J���ϐ��@locals��H���ĕϐ��������Ă������ƂŊ����̕ϐ����ǂ������킩��
extern Variable *globals[];     // �O���[�o���ϐ�
extern int current_func;        // ���[�J���֐��̐�
extern Node *code[];            // �v���O�����̃R�[�h
extern StringToken *strings;    // �����񃊃e����
extern StructTag *struct_tags;  // �\���̃^�O
extern EnumVariable *enum_variables; // enum�̕ϐ�