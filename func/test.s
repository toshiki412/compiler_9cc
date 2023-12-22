# func/func.cppの呼び出し元のアセンブリファイル
# 9ccのコンパイルにfunc.oをとtest.oをリンクするとfunc.cppの関数が呼べる
.intel_syntax noprefix
.globl main
main:
 mov rdi, 3
 mov rsi, 4
 call bar
 ret
