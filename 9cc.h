//ヘッダファイル
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


// グローバル変数
extern char *user_input;        // 入力プログラム
extern char *filename;          // 入力ファイル名
extern Token *token;            // 現在着目しているトークン
extern Variable *locals[];      // ローカル変数　localsを辿って変数名を見ていくことで既存の変数かどうかがわかる
extern Variable *globals[];     // グローバル変数
extern int current_func;        // ローカル関数の数
extern Node *code[];            // プログラムのコード
extern StringToken *strings;    // 文字列リテラル
extern StructTag *struct_tags;  // 構造体タグ
extern EnumVariable *enum_variables; // enumの変数