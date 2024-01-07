//ヘッダファイル
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cstddef>


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


// tokenize.h
//トークンの種類
typedef enum {
TK_RESERVED,    //記号 +, -, *, ==, [, など
TK_IDENT,       //識別子 変数名や関数名
TK_NUM,         //整数トークン
TK_RETURN,      //return
TK_IF,          //if
TK_ELSE,        //else
TK_FOR,         //for
TK_WHILE,       //while
TK_TYPE,        //型
TK_SIZEOF,      //sizeof
TK_STRING,      //文字列
TK_STRUCT,      //structs
TK_TYPEDEF,     //typedef
TK_ENUM,        //enum
TK_BREAK,       //break
TK_CONTINUE,    //continue
TK_SWITCH,      //switch
TK_CASE,        //case
TK_DEFAULT,     //default
TK_EOF,         //入力の終わりを表すトークン
} TokenKind;

//トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMのとき、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ 識別子が一文字だけではなくなった(<, <=)
};

char *read_file(char *path);
void error_at(char *loc, const char *fmt);
void error_at2(char *loc, const char *fmt, char *op);
void error(const char *fmt);
void error2(const char *fmt, const char *op);
void error3(const char *fmt, const char *op, const char *err);
bool consume(const char *op);
bool peek_token_str(const char *op);
Token *consume_kind(TokenKind kind);
void expect(const char *op);
int expect_number();
bool at_eof();
bool startswith(char *p, const char *q);
Token *new_token(TokenKind kind, Token *cur, char *str, int len) ;
Token *tokenize();


// parse.h
//抽象構文木の種類
typedef enum {
    ND_ADD,                 // +
    ND_SUB,                 // -
    ND_MUL,                 // *
    ND_DIV,                 // /
    ND_EQ,                  // ==
    ND_NE,                  // !=
    ND_LT,                  // <
    ND_LE,                  // <=
    ND_ASSIGN,              // =
    ND_LOCAL_VARIABLE,      // ローカル変数
    ND_GLOBAL_VARIABLE_DEF, // グローバル変数の定義 int x; のようなもの
    ND_GLOBAL_VARIABLE_USE, // グローバル変数の使用 x = 1; のようなもの
    ND_NUM,                 // 整数
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_FOR,
    ND_FOR_LEFT,
    ND_FOR_RIGHT,
    ND_WHILE,
    ND_BLOCK,               // {}
    ND_FUNC_CALL,           // 関数呼び出し
    ND_FUNC_DEF,            // 関数定義
    ND_ADDR,                // アドレス &
    ND_DEREF,               // 参照先の値 *
    ND_STRING,              // 文字列
    ND_MEMBER,              // 構造体のメンバ
    ND_BREAK,
    ND_NOT,                 // 論理否定
    ND_BITNOT,              // ビット否定
    ND_BITAND,              // ビットAND
    ND_BITOR,               // ビットOR
    ND_BITXOR,              // ビットXOR
    ND_LOGICAND,            // 論理AND
    ND_LOGICOR,             // 論理OR
    ND_TERNARY,             // 三項演算子
    ND_TERNARY_RIGHT,       // 三項演算子の右辺
    ND_SWITCH,              // switch
    ND_CASE,                // case
    ND_DEFAULT,             // default
    ND_CONTINUE,
} NodeKind;


typedef struct Member Member;
typedef struct Type Type;

struct Member {
    Member *next;
    Type *type;
    char *name;
    int offset;
};

typedef enum {
    CHAR,
    INT,
    PTR,
    ARRAY,
    STRUCT,
} TypeKind;

struct Type {
    // *a[10] は ARRAY -> PTR -> INT のような数珠繋ぎになる
    TypeKind ty;
    struct Type *ptr_to;    // tyがPTRのときポインタの指す先の型
    size_t array_size;      // tyがARRAYのとき配列のサイズ
    Member *member_list;    // tyがSTRUCTのときメンバのリスト
    int byte_size;          // バイトサイズ
    bool is_imcomplete;     // 型のネストが完了していないときtrue
};

typedef struct StringToken StringToken;
struct StringToken {
    int index;
    char *value;
    StringToken *next;
};

//抽象構文木のノードの型
typedef struct Node Node;
typedef struct Variable Variable;
struct Variable {
    Variable *next; // 次の変数かNULL
    char *name;     // 変数の名前
    int len;        // 名前の長さ
    int offset;     // RBPからのオフセット
    Type *type;     // 変数の型
    enum {
        LOCAL_VARIABLE,
        GLOBAL_VARIABLE,
    } kind;
    Node *init_value; // 初期化式のときの値(右辺値)
};

struct Node {
    NodeKind kind;
    Node *lhs;              // 左辺
    Node *rhs;              // 右辺
    Node **block;           // kindがND_BLOCKのとき使う ブロックに含まれる式を持つベクタ
    char *func_name;        // kindがND_FUNCのとき使う
    Node **func_args;       // kindがND_FUNC_DEFのとき使う
    int num_value;          // kindがND_NUMのとき使う
    int offset;             // kindがND_Variableのとき使う
    Type *type;             // kindがND_Variableのとき使う
    char *variable_name;    // kindがND_GlobalVariable, ND_LOCAL_VARIABLEのとき使う
    int byte_size;          // kindがND_GlobalVariableのとき使う
    StringToken *string;    // kindがND_STRINGのとき使う
    Variable *variable;     // kindがND_GLOBAL_VARIABLE_DEFのとき使う
    Member *member;         // kindがND_MEMBERのとき使う
    Node *case_next;        // kindがND_CASEのとき使う
    Node *default_case;     // kindがND_SWITCHのとき使う
    int case_label;         // kindがND_CASEのとき使う
};


typedef struct DefineFuncOrVariable DefineFuncOrVariable;
struct DefineFuncOrVariable {
    Type *type;
    Token *ident;
};

typedef struct StructTag StructTag;
struct StructTag {
    StructTag *next;
    char *name;
    Type *type;
};

typedef struct EnumVariable EnumVariable;
struct EnumVariable {
    EnumVariable *next;
    char *name;
    int value;
};

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int num_value);
Node *new_node_string(StringToken *s);

void program();
Node *func();
Node *stmt();
Node *expr();
Node *assign();
Node *conditional();
Node *logic_or();
Node *logic_and();
Node *bit_or();
Node *bit_xor();
Node *bit_and();
Node *equality();
Node *relational();
Node *add();
Node *ptr_calc(Node *node, Node *right);
Node *mul();
Node *unary();
Node *primary();

Type *get_type(Node *node);
Type *define_struct();
void read_array_type_suffix(DefineFuncOrVariable *def_first_half);
int get_byte_size(Type *type);
Type *read_type();
DefineFuncOrVariable *read_define_first_half();
Node *initialize_local_variable(Node *node);
Node *define_variable(DefineFuncOrVariable *def, Variable **varlist);
Node *variable(Token *tok);
Node *struct_reference(Node *node);
Member *find_member(Token *tok, Type *type);
Variable *find_varable(Token *tok);
int align_to(int n, int align);
void push_struct_tag_to_global(const char* prefix, Token *tok, Type *type, bool is_typedef);
StructTag *find_tag(const char* prefix, Token *tok);
bool define_typedef();
Type *define_enum();
Type *int_type();
Node *find_enum_variable(Token *tok);


// codegen.h
void gen_variable(Node *node);
void gen(Node *node);
