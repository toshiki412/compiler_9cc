#include "9cc.h"

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    // ex) argv[1] : 12 + 31 - 15

    // トークナイズしてぱーずする
    user_input = argv[1];
    token = tokenize();
    Node *node = expr();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //抽象構文木を下りながらコード生成
    gen(node);

    //スタックトップに式全体の値が入っているはずなのでそれをraxにロードして関数からの返り値とする
    printf(" pop rax\n");
    printf(" ret\n");
    return 0;
}