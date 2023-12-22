#include "9cc.h"

//現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

//ローカル変数
LocalVariable *locals[100];
int currentFunc = 0;

//エラーを報告するための関数
//locはエラーが発生した位置
void error_at(char *loc, const char *fmt, ...){
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

void error(const char *fmt, ...){
    va_list ap; //可変引数関数内で可変の引数を操作するためのデータ型
    va_start(ap,fmt); //va_listの初期化、可変引数のアクセスを開始

    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号のときはトークンを一つ進めて真を返す
//それ以外は偽を返す
bool consume(const char *op){
    if( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)){
            return false;
    }
    token = token->next;
    return true;
}

Token *consume_kind(TokenKind kind){
    if(token->kind != kind){
        return NULL;
    }
    Token* tok = token;
    token = token->next;
    return tok;
}

//次のトークンが期待している記号のときはトークンを一つ進める
//それ以外はエラーを報告する
void expect(const char *op){
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

bool startswith(char *p, const char *q){
    return memcmp(p, q, strlen(q)) == 0;
}

bool is_alnum(char c){
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

// curの次に繋げる新しいトークンを作成する
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    //新しいToken構造体のメモリ(RAM)を動的に割り当てる。Token構造体のバイトサイズ×1のバイト数を確保。
    Token *tok = static_cast<Token*>(calloc(1, sizeof(Token)));

    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

typedef struct ReservedWord ReservedWord;
struct ReservedWord{
    const char *word;
    TokenKind kind;
};

ReservedWord reservedWords[] = {
    {"return", TK_RETURN},
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"while", TK_WHILE},
    {"for", TK_FOR},
    {"int", TK_TYPE},
    {"", TK_EOF},
};


// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
    char *input_char_pointer = user_input;
    Token head; //ダミーノード
    head.next = NULL;
    Token *cur = &head;

    // ex)12 + 31 - 15

    while (*input_char_pointer) {
        // 空白文字をスキップ
        if (isspace(*input_char_pointer)) {
        input_char_pointer++;
        continue;
        }

        if (startswith(input_char_pointer, "==") || 
            startswith(input_char_pointer, "!=") ||
            startswith(input_char_pointer, "<=") ||
            startswith(input_char_pointer, ">=")){
                cur = new_token(TK_RESERVED, cur, input_char_pointer, 2);
                input_char_pointer += 2;
                continue;
        }

        if (strchr("+-*/()<>=;{},&", *input_char_pointer)) {
            cur = new_token(TK_RESERVED, cur, input_char_pointer++, 1);
            continue;
        }

        bool found = false;
        for(int i = 0; reservedWords[i].kind != TK_EOF; i++){
            const char *word = reservedWords[i].word;
            int wordLen = strlen(word);
            TokenKind kind = reservedWords[i].kind;

            // pが予約語wまで読んで、かつwの次の文字がアルファベットでない場合
            // w=returnのとき、returnxのようになっていないかを確認
            if(startswith(input_char_pointer, word) && !is_alnum(input_char_pointer[wordLen])){
                cur = new_token(kind, cur, input_char_pointer, wordLen);
                input_char_pointer += wordLen;
                found = true;
                break;
            }
        }
        
        if(found){
            continue;
        }

        if ('a' <= *input_char_pointer && *input_char_pointer <= 'z'){
            char *c = input_char_pointer;
            // 複数文字の変数名を対応
            while(is_alnum(*c)){
                c++;
            }
            int len = c - input_char_pointer;
            cur = new_token(TK_IDENT, cur, input_char_pointer, len);
            input_char_pointer = c;
            continue;
        }

        if (isdigit(*input_char_pointer)) {
            cur = new_token(TK_NUM, cur, input_char_pointer, 0);
            char *p = input_char_pointer;
            cur->val = strtol(input_char_pointer, &input_char_pointer, 10); //文字列からlong型（整数型）に変換
            cur->len = input_char_pointer - p;
            continue;
        }

        error_at(input_char_pointer, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, input_char_pointer, 0);

    // ex )12 + 31 - 15の場合、tokenは以下のように構成されている
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}

// ローカル変数を名前で検索する。見つからなかった場合はNULLを返す。
LocalVariable *find_local_variable(Token *tok){
    for(LocalVariable *var = locals[currentFunc]; var; var = var->next){
        if(var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    return NULL;
}