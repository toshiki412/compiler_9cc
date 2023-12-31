#include "9cc.h"

//現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// 入力ファイル名
char *filename;

char *read_file(char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        error3("cannot open %s: %s", path, strerror(errno));
    }

    // ファイルの長さを調べる
    if (fseek(fp, 0, SEEK_END) == -1) {
        error3("%s: fseek: %s", path, strerror(errno));
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error3("%s: fseek: %s", path, strerror(errno));
    }

    // ファイルの内容を読み込む
    char *buf = static_cast<char*>(calloc(1, size + 2)); // +2は最後にNULLを入れるため

    // fread(読み込む先のポインタ, 1回に読み込むバイト数, 読み込む回数, ファイルストリーム)
    fread(buf, size, 1, fp);

    // ファイルが必ず"\n\0"で終わるようにする
    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';
    fclose(fp);
    return buf;
}


//エラーを報告するための関数
//locはエラーが発生した位置
void error_at2(char *loc, const char *fmt, const char *op) {
    // locが含まれている行の開始地点と終了地点を取得
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }

    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    // 見つかった行が全体の何行目なのかを調べる
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    // 見つかった行を、ファイル名と行番号と一緒に表示
    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    int len = end - line;
    fprintf(stderr, "%.*s\n", len, line);

    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, ""); //posの数だけ空白を出力
    fprintf(stderr, "^ ");
    fprintf(stderr, fmt, op);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, const char *fmt) {
    error_at2(loc, fmt, "");
}

void error3(const char *fmt, const char *op, const char *err) {
    fprintf(stderr, fmt, err);
    fprintf(stderr, "\n");
    exit(1);
}

void error2(const char *fmt, const char *op) {
    fprintf(stderr, fmt, op);
    fprintf(stderr, "\n");
    exit(1);
}

void error(const char *fmt) {
    fprintf(stderr, "%s", fmt);
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号のときはトークンを一つ進めて真を返す
//それ以外は偽を返す
bool consume(const char *op) {
    if ( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
            return false;
    }
    token = token->next;
    return true;
}

// tokenの文字列がopと一致するかどうかを返す
bool peek_token_str(const char *op) {
    return strlen(op) == token->len && memcmp(token->str, op, token->len) == 0;
}

Token *consume_kind(TokenKind kind) {
    if (token->kind != kind) {
        return NULL;
    }
    Token* tok = token;
    token = token->next;
    return tok;
}

//次のトークンが期待している記号のときはトークンを一つ進める
//それ以外はエラーを報告する
void expect(const char *op) {
    if ( token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
            error_at2(token->str, "Expected '%s'", op);
    }
    token = token->next;
}

//次のトークンが数値のとき、トークンを進めてその数値を返す
//それ以外はエラーを報告する
int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str,"Not a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

bool startswith(char *p, const char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

bool is_alnum(char c) {
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

// cがエスケープ文字である場合、エスケープ文字に対応する文字を返す
char get_escape_char(char c) {
    switch (c) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 'v':
            return '\v';
        case 't':
            return '\t';
        case 'e':
            return 27;
        case '0':
            return 0;
        default:
            return c;
    }
}

// 文字リテラルを読み込む
Token *read_char_literal(Token *cur, char *start) {
    char *p = start + 1;
    if (*p == '\0') {
        error_at(start, "unclosed char literal");
    }

    char c;
    if (*p == '\\') {
        p++;
        c = get_escape_char(*p);
        p++;
    } else {
        c = *p;
        p++;
    }
    
    if (*p != '\'') {
        error_at(start, "char literal too long");
    }
    p++;

    Token *tok = new_token(TK_NUM, cur, start, p - start);
    tok->val = c;
    return tok;
}

typedef struct ReservedWord ReservedWord;
struct ReservedWord{
    const char *word;
    TokenKind kind;
};

ReservedWord reserved_words[] = {
    {"return", TK_RETURN},
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"while", TK_WHILE},
    {"for", TK_FOR},
    {"int", TK_TYPE},
    {"char", TK_TYPE},
    {"void", TK_TYPE},
    {"size_t", TK_TYPE},
    {"bool", TK_TYPE},
    {"FILE", TK_TYPE}, // セルフコンパイルのため
    {"sizeof", TK_SIZEOF},
    {"struct", TK_STRUCT},
    {"typedef", TK_TYPEDEF},
    {"enum", TK_ENUM},
    {"break", TK_BREAK},
    {"continue", TK_CONTINUE},
    {"switch", TK_SWITCH},
    {"case", TK_CASE},
    {"default", TK_DEFAULT},
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

        // 行コメントをスキップ
        if (startswith(input_char_pointer, "//")) {
            input_char_pointer += 2;
            while (*input_char_pointer != '\n') {
                input_char_pointer++;
            }
            continue;
        }

        // ブロックコメントをスキップ
        if (startswith(input_char_pointer, "/*")) {
            // strstr(文字列, 検索文字列)は文字列から検索文字列を探し、その先頭へのポインタを返す
            char *p = strstr(input_char_pointer + 2, "*/");
            if (!p) {
                error_at(input_char_pointer, "コメントが閉じられていません");
            }
            input_char_pointer = p + 2;
            continue;
        }

        // includeをスキップ
        if (startswith(input_char_pointer, "#include")) {
            input_char_pointer += 8;
            while (*input_char_pointer != '\n') {
                input_char_pointer++;
            }
            continue;
        }

        // externをスキップ
        if (startswith(input_char_pointer, "extern")) {
            input_char_pointer += 6;
            while (*input_char_pointer != '\n') {
                input_char_pointer++;
            }
            continue;
        }

        // constをスキップ
        if (startswith(input_char_pointer, "const")) {
            input_char_pointer += 5;
            continue;
        }

        // static_castをスキップ
        if (startswith(input_char_pointer, "static_cast")) {
            input_char_pointer += 11;
            while (*input_char_pointer != '>') {
                input_char_pointer++;
            }
            input_char_pointer++;
            continue;
        }

        // const_castをスキップ
        if (startswith(input_char_pointer, "const_cast")) {
            input_char_pointer += 10;
            while (*input_char_pointer != '>') {
                input_char_pointer++;
            }
            input_char_pointer++;
            continue;
        }

        if (startswith(input_char_pointer, "==") || 
            startswith(input_char_pointer, "!=") ||
            startswith(input_char_pointer, "<=") ||
            startswith(input_char_pointer, ">=") ||
            startswith(input_char_pointer, "+=") ||
            startswith(input_char_pointer, "-=") ||
            startswith(input_char_pointer, "*=") ||
            startswith(input_char_pointer, "/=") ||
            startswith(input_char_pointer, "++") ||
            startswith(input_char_pointer, "--") ||
            startswith(input_char_pointer, "||") ||
            startswith(input_char_pointer, "&&") ||
            startswith(input_char_pointer, "->")) {
                cur = new_token(TK_RESERVED, cur, input_char_pointer, 2);
                input_char_pointer += 2;
                continue;
        }

        if (*input_char_pointer == '\'') {
            cur = read_char_literal(cur, input_char_pointer);
            input_char_pointer += cur->len;
            continue;
        }

        if (strchr("+-*/()<>=;{},&[].!~|^?:", *input_char_pointer)) {
            cur = new_token(TK_RESERVED, cur, input_char_pointer++, 1);
            continue;
        }

        bool found = false;
        for (int i = 0; reserved_words[i].kind != TK_EOF; i++) {
            const char *word = reserved_words[i].word;
            int word_len = strlen(word);
            TokenKind kind = reserved_words[i].kind;

            // pが予約語wまで読んで、かつwの次の文字がアルファベットでない場合
            // w=returnのとき、returnxのようになっていないかを確認
            if (startswith(input_char_pointer, word) && !is_alnum(input_char_pointer[word_len])) {
                cur = new_token(kind, cur, input_char_pointer, word_len);
                input_char_pointer += word_len;
                found = true;
                break;
            }
        }
        
        if (found) {
            continue;
        }

        if ('a' <= *input_char_pointer && *input_char_pointer <= 'z'
            || 'A' <= *input_char_pointer && *input_char_pointer <= 'Z') {
            char *c = input_char_pointer;
            // 複数文字の変数名を対応
            while (is_alnum(*c)) {
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

        // string literalの場合
        if ('"' == *input_char_pointer) {
            // "HELLO"の場合、input_char_pointerは"を指している

            // input_char_pointerをHにする
            input_char_pointer++;

            // cが最後の"を指すまで進める
            char *c = input_char_pointer;

            while (true) {
                // \" の場合、cは"を指している
                if (startswith(c, "\\\"")) {
                    c += 2;
                    continue;
                }

                if (*c == '"') {
                    break;
                }

                c++;
            }

            int len = c - input_char_pointer; // lenはHELLOの文字数

            cur = new_token(TK_STRING, cur, input_char_pointer, len);

            // input_char_pointerは"の次の文字を指す
            input_char_pointer = c + 1;
            continue;
        }

        error_at(input_char_pointer, "Cannot tokenize");
    }

    new_token(TK_EOF, cur, input_char_pointer, 0);

    // ex )12 + 31 - 15の場合、tokenは以下のように構成されている
    // &head -> TK_NUM("12") -> TK_RESERVED("+") -> TK_NUM("31") -> TK_RESERVED("-") -> TK_NUM("15") -> TK_EOF
    return head.next;
}