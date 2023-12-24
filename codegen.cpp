#include "9cc.h"

void gen_left_value(Node *node) {
    // derefである場合 *p = 1; など
    if (node->kind == ND_DEREF) {
        gen(node->lhs);
        return;
    }

    if (node->kind != ND_LOCAL_VARIABLE) {
        error("not ND_LOCAL_VARIABLE");
    }

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

int genCounter = 0; //genが呼ばれるたびにインクリメントされる

//x86-64のABIに従い、引数はとりあえず6つまで
const char *argRegisters[] = {"rdi", "rsi", "rdx", "rcx", "r8","r9"}; 

//スタックマシン
void gen(Node *node) {
    if (!node) return;
    genCounter += 1;
    int labelId = genCounter;
    int functionArgNum = 0;

    switch (node->kind) {
    case ND_ADDR:
        gen_left_value(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_FUNC_DEF:
        printf("%s:\n", node->funcName);

        //プロローグ
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        //引数の値をスタックに積む
        for (int i = 0; node->funcArgs[i]; i++) {
            printf("    push %s\n", argRegisters[i]);
            functionArgNum++;
        }
        //引数の数を除いた変数の数だけrspをずらして、変数領域を確保する。
        if (locals[currentFunc]) {
            // 全体のずらすべき数
            int offset = locals[currentFunc]->offset;

            for (LocalVariable *cur = locals[currentFunc]; cur; cur = cur->next) {
                // TODO 現在はint型のみ対応
                offset += cur->type->ty == Type::ARRAY ? cur->type->array_size * 4 : 8;
            }

            // スタックに積んだ引数の数を除いた数
            offset -= functionArgNum * 8;
            
            printf("    sub rsp, %d\n", offset);
        }

        gen(node->lhs);

        //エピローグ
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");

        return;
    case ND_FUNC_CALL:
        for (int i = 0; node->block[i]; i++) {
            gen(node->block[i]);
            functionArgNum++;
        }

        //スタックに積んだ引数の値を取り出す
        for (int i = functionArgNum - 1; i >= 0; i--) {
            printf("    pop %s\n", argRegisters[i]);
        }
        
        //関数呼び出し
        printf("    mov rax, rsp\n");
        printf("    and rax, 15\n");                //下位４bitをマスクする rspが16の倍数かどうかをチェックする
        printf("    jnz .L.call.%03d\n", labelId);  //16の倍数じゃないならばジャンプ
        printf("    mov rax, 0\n");
        printf("    call %s\n", node->funcName);    //関数呼び出し
        printf("    jmp .L.end.%03d\n", labelId);
        printf(".L.call.%03d:\n", labelId);
        printf("    sub rsp, 8\n");                 // rspを16の倍数にするために8バイト分ずらす
        printf("    mov rax, 0\n");
        printf("    call %s\n", node->funcName);    //関数呼び出し
        printf("    add rsp, 8\n");                 // 8バイトずらしたrspを元に戻す
        printf(".L.end.%03d:\n", labelId);
        printf("    push rax\n");                   //関数からリターンしたときにraxに入っている値が関数の返り値という約束

        return;
    case ND_BLOCK:
        // ND_BLOCKに含まれるステートメントのコードを順番に生成する
        for (int i = 0; node->block[i]; i++) {
            gen(node->block[i]);
            // printf("    pop rax\n");
        }
        return;
    case ND_FOR:
        //for (A;B;C) D;
        gen(node->lhs->lhs);        //Aをコンパイルしたコード
        printf(".Lbegin%03d:\n", labelId);
        gen(node->lhs->rhs);        //Bをコンパイルしたコード
        if (!node->lhs->rhs) {        //無限ループの対応
            printf("    push 1\n");
        }
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%03d\n", labelId);
        gen(node->rhs->rhs);        //Dをコンパイルしたコード
        gen(node->rhs->lhs);        //Cをコンパイルしたコード 
        printf("    jmp .Lbegin%03d\n", labelId);
        printf(".Lend%03d:\n", labelId);
        return;
    case ND_WHILE:
        //while (A) B;
        printf(".Lbegin%03d:\n", labelId);
        gen(node->lhs);             //Aをコンパイルしたコード
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%03d\n", labelId);
        gen(node->rhs);             //Bをコンパイルしたコード
        printf("    jmp .Lbegin%03d\n", labelId);
        printf(".Lend%03d:\n", labelId);
        return;
    case ND_IF:
        //if (A) B; else C;
        gen(node->lhs);             //Aをコンパイルしたコード
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lelse%03d\n", labelId);
        if (node->rhs->kind == ND_ELSE) {
            gen(node->rhs->lhs);    //Bをコンパイルしたコード
        } else {
            gen(node->rhs);         //Bをコンパイルしたコード
        }
        printf("    jmp .Lend%03d\n", labelId);
        printf(".Lelse%03d:\n", labelId);
        if (node->rhs->kind == ND_ELSE) {
            gen(node->rhs->rhs);    //Cをコンパイルしたコード
        }
        printf(".Lend%03d:\n", labelId);
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_NUM:
        printf("    push %d\n", node->val);
        return;
    case ND_LOCAL_VARIABLE:
        gen_left_value(node);
        if (node->type && node->type->ty == Type::ARRAY) {
            return;
        }
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_ASSIGN:
        gen_left_value(node->lhs);
        gen(node->rhs);
        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
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
    }
    
    printf("    push rax\n");
}

