#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//トークンの種類
typedef enum{
TK_RESERVED,    //記号
TK_NUM,         //整数トークン
TK_EOF,         //入力の終わりを表すトークン
} TokenKind;

//抽象構文木の種類
typedef enum{
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
} NodeKind;

typedef struct Token Token;

typedef struct Node Node;

//トークン型
struct Token{
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMのとき、その数値
    char *str;      //トークン文字列
};

//抽象構文木のノードの型
struct Node{
    NodeKind kind;
    Node *lhs; //左辺
    Node *rhs; //右辺
    int val;   //kindがND_NUMのとき使う
};

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

//次のトークンが期待している記号のときはトークンを一つ進めて真を返す
//それ以外は偽を返す
bool consume(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        return false;
    }
    token = token->next;
    return true;
}

//次のトークンが期待している記号のときはトークンを一つ進める
//それ以外はエラーを報告する
void expect(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        error_at(token->str, "Not '%c'",op);
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

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = static_cast<Token*>(calloc(1, sizeof(Token)));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs){
    Node *node = static_cast<Node*>(calloc(1, sizeof(Token)));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs; 
    return node;
}

Node *new_node_num(int val){
    Node *node = static_cast<Node*>(calloc(1, sizeof(Token)));
    node->kind = ND_NUM;
    node->val = val;
    return node;
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

        if (strchr("+-*/()", *p)) {
        cur = new_token(TK_RESERVED, cur, p++);
        continue;
        }

        if (isdigit(*p)) {
        cur = new_token(TK_NUM, cur, p);
        cur->val = strtol(p, &p, 10); //文字列からlong型（整数型）に変換
        continue;
        }

        error_at(p, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, p);

    // ex )12 + 31 - 15の場合、tokenは以下のように構成されている
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}

Node *expr();  // expr 関数のプロトタイプ
Node *mul();   // mul 関数のプロトタイプ
Node *primary();  // primary 関数のプロトタイプ

Node *primary() {
    // primary = num | "(" expr ")"

    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume('(')) {
        Node *node = expr();
        expect(')');
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

Node *mul() {
    // mul     = primary ("*" primary | "/" primary)*

    Node *node = primary();
    for (;;) {
        if (consume('*')){
            node = new_node(ND_MUL, node, primary());
        }else if (consume('/')){
            node = new_node(ND_DIV, node, primary());
        }else{
            return node;
        }
    }
}

Node *expr(){
    // expr    = mul ("+" mul | "-" mul)*

    Node *node = mul();
    for(;;){
        if (consume('+')){
            node = new_node(ND_ADD, node, mul());
        }else if(consume('-')){
            node = new_node(ND_SUB, node, mul());
        }else{
            return node;
        }
    }
}

//スタックマシン
void gen(Node *node){
    if(node->kind == ND_NUM){
        printf(" push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf(" pop rdi\n");
    printf(" pop rax\n");

    switch (node->kind){
    case ND_ADD:
        printf(" add rax, rdi\n");
        break;
    case ND_SUB:
        printf(" sub rax, rdi\n");
        break;
    case ND_MUL:
        printf(" imul rax, rdi\n");
        break;
    case ND_DIV:
        printf(" cqo\n");
        printf(" idiv rdi\n");
        break;
    }
    
    printf(" push rax\n");
}

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