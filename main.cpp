#include "9cc.h"

int main(int argc, char **argv) {
    // 複数の入力ファイル読み込み
    for (int i = 1; i < argc; i++) {
        filename = argv[i];
        user_input = read_file(filename);

        // トークナイズ
        Token *t = tokenize();
        if (!token) {
            token = t; // 最初のファイルのトークンをtokenにセット
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


    // パース
    program();

    // コード生成
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".bss\n"); // データセグメントの開始(グローバル変数の定義)
    
    // グローバル変数の定義 初期化式がない場合
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_GLOBAL_VARIABLE_DEF && !code[i]->variable->init_value) {
            gen(code[i]);
        }
    }

    printf(".data\n"); // データセグメントの開始(文字列リテラルの定義)
    // グローバル変数の定義 初期化式がある場合
    for (int i = 0; code[i]; i++) {
        if (code[i]->kind == ND_GLOBAL_VARIABLE_DEF && code[i]->variable->init_value) {
            gen(code[i]);
        }
    }

    // 文字列リテラルの定義
    for (StringToken *s = strings; s; s = s->next) {
        printf(".LC_%d:\n", s->index);
        printf("    .string \"%s\"\n", s->value);
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