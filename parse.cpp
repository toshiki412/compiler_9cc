#include "9cc.h"

//現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

//エラーを報告するための関数
//locはエラーが発生した位置
void error_at(char *loc, char *fmt, ...){
    va_list ap; //可変引数関数内で可変の引数を操作するためのデータ型
    va_start(ap,fmt); //va_listの初期化、可変引数のアクセスを開始

    //posはlocがuser_inputからどれだけ離れているかを表すオフセット
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
    //printfの出力先は標準出力、fprintfの出力先は指定したファイルストリーム、vfprintfは可変引数を与えている
}

void error(char *fmt, ...){
    va_list ap; //可変引数関数内で可変の引数を操作するためのデータ型
    va_start(ap,fmt); //va_listの初期化、可変引数のアクセスを開始

    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号のときはトークンを一つ進めて真を返す
//それ以外は偽を返す
bool consume(char *op){
    if( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)){
            return false;
    }
    token = token->next;
    return true;
}

Token* consume_ident(){
    if(token->kind != TK_IDENT){
        return NULL;
    }
    Token* tok = token;
    token = token->next;
    return tok;
}

//次のトークンが期待している記号のときはトークンを一つ進める
//それ以外はエラーを報告する
void expect(char *op){
    if( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)){
            error_at(token->str, "Expected \"%s\"", op);
    }
    token = token->next;
}

//次のトークンが数値のとき、トークンを進めてその数値を返す
//それ以外はエラーを報告する
int expect_number(){
    if(token->kind != TK_NUM){
        error_at(token->str,"Not a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(){
    return token->kind == TK_EOF;
}

bool startswith(char *p, char *q){
    return memcmp(p, q, strlen(q)) == 0;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    //新しいToken構造体のメモリ(RAM)を動的に割り当てる。Token構造体のバイトサイズ×1のバイト数を確保。
    Token *tok = static_cast<Token*>(calloc(1, sizeof(Token)));

    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
    char *p = user_input;
    Token head; //ダミーノード
    head.next = NULL;
    Token *cur = &head;

    // ex)12 + 31 - 15

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
        p++;
        continue;
        }

        if (startswith(p, "==") || 
            startswith(p, "!=") ||
            startswith(p, "<=") ||
            startswith(p, ">=")){
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                continue;
        }

        if (strchr("+-*/()<>=;", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if ('a' <= *p && *p <= 'z'){
            cur = new_token(TK_IDENT, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10); //文字列からlong型（整数型）に変換
            cur->len = p - q;
            continue;
        }

        error_at(p, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, p, 0);

    // ex )12 + 31 - 15の場合、tokenは以下のように構成されている
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}
