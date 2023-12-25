//ヘッダファイル
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenize.h"
#include "codegen.h"
#include "parse.h"


// 入力プログラム
extern char *user_input;

//現在着目しているトークン
extern Token *token;

// ローカル変数
// localsを辿って変数名を見ていくことで既存の変数かどうかがわかる
extern Variable *locals[];

extern Variable *globals[];

extern int current_func;
extern Node *code[];
extern StringToken *strings;