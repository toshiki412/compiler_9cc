#include "9cc.h"

// �ϐ��̃A�h���X���X�^�b�N�ɐς�
void gen_variable(Node *node) {
    // deref�ł���ꍇ *p = 1; �Ȃ�
    if (node->kind == ND_DEREF) {
        gen(node->lhs);
        return;
    }

    if (node->kind == ND_LOCAL_VARIABLE) {
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", node->offset);
        printf("    push rax\n");
    } else if (node->kind == ND_GLOBAL_VARIABLE_USE) {
        printf("    push offset %s\n", node->variable_name);
    } else if (node->kind == ND_MEMBER) {
        gen_variable(node->lhs);
        printf("    pop rax\n");
        printf("    add rax, %d\n", node->member->offset);
        printf("    push rax\n");
    } else {
        error("not VARIABLE");
    }
}

int gen_counter = 0; //gen���Ă΂�邽�тɃC���N�������g�����
int break_id = 0;    //break�̃W�����v��̃��x���ԍ�
int conitnue_id = 0; //continue�̃W�����v��̃��x���ԍ�

//x86-64��ABI�ɏ]���A�����͂Ƃ肠����6�܂�
const char *arg_registers_8bit[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"}; 
const char *arg_registers_16bit[] = {"di", "si", "dx", "cx", "r8w", "r9w"}; 
const char *arg_registers_32bit[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"}; 
const char *arg_registers_64bit[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"}; 

//�X�^�b�N�}�V��
void gen(Node *node) {
    if (!node) return;
    gen_counter += 1;
    int label_id = gen_counter;
    int func_arg_num = 0;
    Type *type; //�ϐ��̌^
    int bid = 0;    //break_id��ޔ����邽�߂̕ϐ�
    int cid = 0;    //continue_id��ޔ����邽�߂̕ϐ�

    switch (node->kind) {
    case ND_STRING:
    printf("    push offset .LC_%d\n", node->string->index);
        return;
    case ND_ADDR:
        gen_variable(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        type = get_type(node);

        if (type->ty == ARRAY) {
            return;
        }
        
        printf("    pop rax\n");
        if (type && type->ty == CHAR) {
            printf("    movsx rax, BYTE PTR [rax]\n");
        } else if (type && type->ty == INT) {
            printf("    movsxd rax, DWORD PTR [rax]\n");
        } else {
            printf("    mov rax, [rax]\n");
        }
        printf("    push rax\n");
        return;
    case ND_FUNC_DEF:
        printf(".global %s\n", node->func_name);
        printf("%s:\n", node->func_name);

        //�v�����[�O
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        //�����̐����������ϐ��̐�����rsp�����炵�āA�ϐ��̈���m�ۂ���B
        if (locals[current_func]) {
            int offset = locals[current_func]->offset;
            printf("    sub rsp, %d\n", offset);
        }

        //�����̒l���X�^�b�N�ɐς�
        for (int i = 0; node->func_args[i]; i++) {
            if (node->func_args[i]->byte_size == 1) {
                printf("    mov [rbp-%d], %s\n", node->func_args[i]->offset, arg_registers_8bit[i]);
            } else if (node->func_args[i]->byte_size == 2) {
                printf("    mov [rbp-%d], %s\n", node->func_args[i]->offset, arg_registers_16bit[i]);
            } else if (node->func_args[i]->byte_size == 4) {
                printf("    mov [rbp-%d], %s\n", node->func_args[i]->offset, arg_registers_32bit[i]);
            } else if (node->func_args[i]->byte_size == 8) {
                printf("    mov [rbp-%d], %s\n", node->func_args[i]->offset, arg_registers_64bit[i]);
            } else {
                error("invalid byte size");
            }
            func_arg_num++;
        }

        gen(node->lhs);

        //�G�s���[�O
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");

        return;
    case ND_FUNC_CALL:
        for (int i = 0; node->block[i]; i++) {
            gen(node->block[i]);
            func_arg_num++;
        }

        //�X�^�b�N�ɐς񂾈����̒l�����o��
        for (int i = func_arg_num - 1; i >= 0; i--) {
            printf("    pop %s\n", arg_registers_64bit[i]);
        }
        
        //�֐��Ăяo��
        printf("    mov rax, rsp\n");
        printf("    and rax, 15\n");                //���ʂSbit���}�X�N���� rsp��16�̔{�����ǂ������`�F�b�N����
        printf("    jnz .L.call.%03d\n", label_id);  //16�̔{������Ȃ��Ȃ�΃W�����v
        printf("    mov rax, 0\n");
        printf("    call %s\n", node->func_name);    //�֐��Ăяo��
        printf("    jmp .L.end.%03d\n", label_id);
        printf(".L.call.%03d:\n", label_id);
        printf("    sub rsp, 8\n");                 // rsp��16�̔{���ɂ��邽�߂�8�o�C�g�����炷
        printf("    mov rax, 0\n");                 // al��0�ɂ���
        printf("    call %s\n", node->func_name);    //�֐��Ăяo��
        printf("    add rsp, 8\n");                 // 8�o�C�g���炵��rsp�����ɖ߂�
        printf(".L.end.%03d:\n", label_id);
        printf("    push rax\n");                   //�֐����烊�^�[�������Ƃ���rax�ɓ����Ă���l���֐��̕Ԃ�l�Ƃ�����

        return;
    case ND_BLOCK:
        // ND_BLOCK�Ɋ܂܂��X�e�[�g�����g�̃R�[�h�����Ԃɐ�������
        for (int i = 0; node->block[i]; i++) {
            gen(node->block[i]);
            // printf("    pop rax\n");
        }
        return;
    case ND_FOR:
        bid = break_id; //break_id��ޔ�
        break_id = label_id;

        cid = conitnue_id; //conitnue_id��ޔ�
        conitnue_id = label_id;

        //for (A;B;C) D;
        gen(node->lhs->lhs);        //A���R���p�C�������R�[�h
        printf(".L.begin%03d:\n", label_id);
        printf(".L.continue%03d:\n", label_id);
        gen(node->lhs->rhs);        //B���R���p�C�������R�[�h
        if (!node->lhs->rhs) {        //�������[�v�̑Ή�
            printf("    push 1\n");
        }
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .L.end%03d\n", label_id);
        gen(node->rhs->lhs);        //C���R���p�C�������R�[�h 
        gen(node->rhs->rhs);        //D���R���p�C�������R�[�h
        printf("    jmp .L.begin%03d\n", label_id);
        printf(".L.end%03d:\n", label_id);

        break_id = bid;
        conitnue_id = cid;
        return;
    case ND_WHILE:
        bid = break_id; //break_id��ޔ�
        break_id = label_id;

        cid = conitnue_id; //conitnue_id��ޔ�
        conitnue_id = label_id;

        //while (A) B;
        printf(".L.begin%03d:\n", label_id);
        printf(".L.continue%03d:\n", label_id);
        gen(node->lhs);             //A���R���p�C�������R�[�h
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .L.end%03d\n", label_id);
        gen(node->rhs);             //B���R���p�C�������R�[�h
        printf("    jmp .L.begin%03d\n", label_id);
        printf(".L.end%03d:\n", label_id);

        break_id = bid;
        conitnue_id = cid;
        return;
    case ND_IF:
        //if (A) B; else C;
        gen(node->lhs);             //A���R���p�C�������R�[�h
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .L.else%03d\n", label_id);
        if (node->rhs->kind == ND_ELSE) {
            gen(node->rhs->lhs);    //B���R���p�C�������R�[�h
        } else {
            gen(node->rhs);         //B���R���p�C�������R�[�h
        }
        printf("    jmp .L.end%03d\n", label_id);
        printf(".L.else%03d:\n", label_id);
        if (node->rhs->kind == ND_ELSE) {
            gen(node->rhs->rhs);    //C���R���p�C�������R�[�h
        }
        printf(".L.end%03d:\n", label_id);
        return;
    case ND_BREAK:
        if (break_id == 0) {
            error("stray break");
        }
        printf("    jmp .L.end%03d\n", break_id);
        return;
    case ND_CONTINUE:
        if (conitnue_id == 0) {
            error("stray continue");
        }
        printf("    jmp .L.continue%03d\n", conitnue_id);
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_NUM:
        printf("    push %d\n", node->num_value);
        return;
    case ND_MEMBER:
    case ND_LOCAL_VARIABLE:
    case ND_GLOBAL_VARIABLE_USE:
        gen_variable(node);
        type = get_type(node);
        if (type && type->ty == ARRAY) {
            return;
        }
        printf("    pop rax\n");
        if (type && type->ty == CHAR) {
            printf("    movsx rax, BYTE PTR [rax]\n");
        } else if (type && type->ty == INT) {
            printf("    movsxd rax, DWORD PTR [rax]\n");
        } else {
            printf("    mov rax, [rax]\n");
        }
        printf("    push rax\n");
        return;
    case ND_GLOBAL_VARIABLE_DEF:
        printf("%s:\n", node->variable_name);
        // �����������Ȃ��ꍇ
        if (node->variable->init_value == NULL) {
            printf("    .zero %d\n", node->byte_size);
            return;
        }

        // �z��̏��������̏ꍇ
        if (node->type->ty == ARRAY && node->variable->init_value->block) {
            for (int i = 0; node->variable->init_value->block[i]; i++) {
                switch (node->type->ptr_to->ty) {
                case INT:
                    printf("    .long 0x%x\n", node->variable->init_value->block[i]->num_value);
                    break;
                case CHAR:
                    printf("    .byte 0x%x\n", node->variable->init_value->block[i]->num_value);
                    break;
                case PTR:
                    printf("    .quad .LC_%d\n", node->variable->init_value->block[i]->string->index);
                    break;
                default:
                    break;
                }
            }
            return;
        }

        // char *g = "abc"; �̂悤�ȏꍇ �A�Z���u���͈ȉ��̂悤�ɂȂ�
        // .LC_1:
        //     .string "abc"
        // g:
        //     .quad .LC_1
        if (node->variable->init_value->kind == ND_STRING) {
            if (node->type->ty == ARRAY) {
                printf("    .string \"%s\"\n", node->variable->init_value->string->value);
            } else {
                printf("    .quad .LC_%d\n", node->variable->init_value->string->index);
            }
            return;
        }

        // �萔���̏�������������ꍇ
        printf("    .long 0x%x\n", node->variable->init_value->num_value);
        return;
    case ND_ASSIGN:
        gen_variable(node->lhs);
        gen(node->rhs);
        type = get_type(node);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        if (type && type->ty == CHAR) {
            printf("    mov [rax], dil\n");
        } else if (type && type->ty == INT) {
            printf("    mov [rax], edi\n");
        } else {
            printf("    mov [rax], rdi\n");
        }
        printf("    push rdi\n");
        return;
    case ND_NOT:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        printf("    push rax\n");
        return;
    case ND_BITNOT:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    not rax\n");
        printf("    push rax\n");
        return;
    case ND_LOGICAND:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .L.false%03d\n", label_id);
        gen(node->rhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .L.false%03d\n", label_id);
        printf("    push 1\n");
        printf("    jmp .L.end%03d\n", label_id);
        printf(".L.false%03d:\n", label_id);
        printf("    push 0\n");
        printf(".L.end%03d:\n", label_id);
        return; // break�łȂ�return�ɂ��邱�ƂŁA����gen�ɍs���Ȃ��悤�ɂ���
    case ND_LOGICOR:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    jne .L.true%03d\n", label_id);
        gen(node->rhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    jne .L.true%03d\n", label_id);
        printf("    push 0\n");
        printf("    jmp .L.end%03d\n", label_id);
        printf(".L.true%03d:\n", label_id);
        printf("    push 1\n");
        printf(".L.end%03d:\n", label_id);
        return; // break�łȂ�return�ɂ��邱�ƂŁA����gen�ɍs���Ȃ��悤�ɂ���
    case ND_TERNARY:
        // �O�����Z�q
        gen(node->lhs); // ������
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .L.else%03d\n", label_id);
        gen(node->rhs->lhs); // ���������^�̏ꍇ
        printf("    jmp .L.end%03d\n", label_id);
        printf(".L.else%03d:\n", label_id);
        gen(node->rhs->rhs); // ���������U�̏ꍇ
        printf(".L.end%03d:\n", label_id);
        return;
    case ND_SWITCH:
        bid = break_id; //break_id��ޔ�
        break_id = label_id;
        node->case_label = label_id;

        //switch (A) B;
        gen(node->lhs);             //A���R���p�C�������R�[�h
        printf("    pop rax\n");

        for (Node *n = node->case_next; n; n = n->case_next) {
            gen_counter += 1;
            n->case_label = gen_counter;
            printf("    cmp rax, %d\n", n->num_value);
            printf("    je .L.case%03d\n", n->case_label);
        }

        if (node->default_case) {
            gen_counter += 1;
            int i = gen_counter;
            node->default_case->case_label = i;
            printf("    jmp .L.case%03d\n", i);
        }

        printf("    jmp .L.end%03d\n", label_id);
        gen(node->rhs);             //B���R���p�C�������R�[�h
        printf(".L.end%03d:\n", label_id);

        break_id = bid;
        return;
    case ND_CASE:
    case ND_DEFAULT:
        printf(".L.case%03d:\n", node->case_label);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    case ND_EQ:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LT:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_BITAND:
        printf("    and rax, rdi\n");
        break;
    case ND_BITOR:
        printf("    or rax, rdi\n");
        break;
    case ND_BITXOR:
        printf("    xor rax, rdi\n");
        break;
    }

    
    printf("    push rax\n");
}

