#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    user_input = argv[1];

    // トークナイズ
    token = tokenize();

    // パース
    program();

    // コード生成
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".bss\n"); // データセグメントの開始(グローバル変数の定義)
    
    // グローバル変数の定義
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_GLOBAL_VARIABLE_DEF) {
            gen(code[i]);
        }
    }

    printf(".text\n"); // プログラムの実行部分の開始
    current_func = 0;
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_FUNC_DEF) {
            current_func++;
            gen(code[i]);
        }
    }

    return 0;
}