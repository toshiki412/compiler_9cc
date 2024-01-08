#include "9cc.h"

// ローカル変数 100個の関数まで対応
Variable *locals[100];

// グローバル変数
Variable *globals[100];

int current_func = 0;

StringToken *strings;

int struct_def_index = 0;
StructTag *struct_tags;

EnumVariable *enum_variables;

// ネストしたswitch文の場合、switch文のノードを保持する
Node *current_switch = NULL;

Node *new_node(NodeKind kind) {
    Node *node = static_cast<Node*>(calloc(1, sizeof(Node)));
    node->kind = kind;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs; 
    return node;
}

Node *new_node_num(int num_value) {
    Node *node = new_node(ND_NUM);
    node->num_value = num_value;
    return node;
}

Node *new_node_string(StringToken *s) {
    Node *node = new_node(ND_STRING);
    node->string = s;
    return node;
}

// 100行までしか対応していない
Node *code[100];

// trueやfalse, errorなどの定数を定義する
void define_constant(const char *name, int value) {
    EnumVariable *ev = static_cast<EnumVariable*>(calloc(1,sizeof(EnumVariable)));
    ev->name = const_cast<char*>(name);
    // ev->name = name;
    ev->value = value;
    ev->next = enum_variables;
    enum_variables = ev;
}

// 9cc.hに構文あり
void program() {
    define_constant("errno", 0);
    define_constant("SEEK_END", 2);
    define_constant("SEEK_SET", 0);
    define_constant("stderr", 0);
    define_constant("NULL", 0);
    define_constant("false", 0);
    define_constant("true", 1);
    int i = 0;
    while (!at_eof()) {
        Node *n = func();
        if (!n) {
            continue;
        }
        code[i++] = n;
    }
    code[i] = NULL;
}

Node *func() {
    Node *node;
    Type *type = NULL;

    if (define_typedef()) {
        return NULL;
    }

    type = define_enum();
    if (type) {
        expect(";");
        return NULL;
    }


    type = define_struct();
    if (type) {
        expect(";");
        return NULL;
    }

    // int *foo() {} のような関数定義の場合, int *fooの部分を読む
    DefineFuncOrVariable *def_first_half = read_define_first_half();

    // 引数の中身を読む
    if (consume("(")) {
        current_func++;

        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_FUNC_DEF;
        node->func_name = static_cast<char*>(calloc(100,sizeof(char)));
        memcpy(node->func_name, def_first_half->ident->str, def_first_half->ident->len);
        node->func_args = static_cast<Node**>(calloc(10,sizeof(Node*))); //引数10個分の配列の長さを作る

        for (int i = 0; !consume(")"); i++) {
            node->func_args[i] = define_variable(read_define_first_half(), locals);

            if (consume(")")) {
                break;
            }
            expect(",");
        }

        // プロトタイプ宣言の場合は読み飛ばす
        if(consume(";")) {
            //localsを消す必要あり
            locals[current_func] = NULL;
            current_func--;
            return NULL;
        }

        node->lhs = stmt();
        return node;
    } else {
        // fooのあと( でなければ変数定義である
        node = define_variable(def_first_half, globals); // グローバル変数の登録
        node->kind = ND_GLOBAL_VARIABLE_DEF; // グローバル変数の場合はND_GLOBAL_VARIABLE_DEFに書き換える ちょっと微妙
        expect(";");
        return node;
    }
}

Node *stmt() {
    Node *node;

    if (consume("{")) {
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_BLOCK;
        // 100行までしか対応していない
        node->block = static_cast<Node**>(calloc(100,sizeof(Node)));
        for (int i = 0; !consume("}"); i++) {
            node->block[i] = stmt(); // {}内にあるstmtを追加
        }
        return node;
    }

    if (consume_kind(TK_FOR)) {
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_FOR;

        Node *left = static_cast<Node*>(calloc(1,sizeof(Node)));
        left->kind = ND_FOR_LEFT;
        Node *right = static_cast<Node*>(calloc(1,sizeof(Node)));
        right->kind = ND_FOR_RIGHT;

        if (!consume(";")) {
            left->lhs = stmt(); // expr()だと、int a = 0;のような場合に対応できない. しかしforの中にforもできてしまう
            // expect(";");  // stmt()の中で読んでいるので、ここでは読まない
        }

        if (!consume(";")) {
            left->rhs = expr();
            expect(";");
        }

        if (!consume(")")) {
            right->lhs = expr();
            expect(")");
        }
        
        right->rhs = stmt();

        node->lhs = left;
        node->rhs = right;
        return node;
    }

    if (consume_kind(TK_WHILE)) {
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_WHILE;
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        return node;
    }

    if (consume_kind(TK_IF)) {
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_IF;
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        if (consume_kind(TK_ELSE)) {
            Node *els = static_cast<Node*>(calloc(1,sizeof(Node)));
            els->kind = ND_ELSE;
            els->lhs = node->rhs;
            els->rhs = stmt();
            node->rhs = els;
        }
        return node;
    }

    if (consume_kind(TK_RETURN)) {
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_RETURN;
        if (consume(";")) { // return; の場合0を補完するようにしておく
            node->lhs = new_node_num(0);
            return node;
        }
        node->lhs = expr();
        expect(";");
        return node;
    }

    if (consume_kind(TK_BREAK)) {
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_BREAK;
        expect(";");
        return node;
    }

    if (consume_kind(TK_CONTINUE)) {
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_CONTINUE;
        expect(";");
        return node;
    }

    if (consume_kind(TK_SWITCH)) {
        // switch (A) B
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_SWITCH;
        node->lhs = expr(); // A
        expect(")");

        Node *sw = current_switch;
        current_switch = node;

        node->rhs = stmt(); // B

        current_switch = sw;
        return node;
    }

    if (consume_kind(TK_CASE)) {
        if (!current_switch) {
            error("case label not within a switch statement.");
        }

        Token *t = token;
        Token *ident = consume_kind(TK_IDENT);
        int value;
        Node *n = NULL;
        if (ident) {
            n = find_enum_variable(ident);
        }
        if (n) { // case x: のxがenumの場合
            value = find_enum_variable(ident)->num_value;
        } else { // 数字の場合
            token = t;
            value = expect_number();
        }

        expect(":");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_CASE;
        node->num_value = value;

        node->case_next = current_switch->case_next;
        current_switch->case_next = node;
        return node;
    }

    if (consume_kind(TK_DEFAULT)) {
        if (!current_switch) {
            error("default label not within a switch statement.");
        }
        expect(":");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_DEFAULT;
        current_switch->default_case = node;
        return node;
    }

    DefineFuncOrVariable *def_first_half = read_define_first_half();
    if (def_first_half) {
        node = define_variable(def_first_half, locals);
        node = initialize_local_variable(node);
        expect(";");
        return node;
    }

    node = expr();
    expect(";");
    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = conditional();
    if (consume("=")) {
        node = new_binary(ND_ASSIGN, node, assign());
    }

    if (consume("*=")) {
        Node *mul = new_binary(ND_MUL, node, assign());
        node = new_binary(ND_ASSIGN, node, mul);
    }

    if (consume("/=")) {
        Node *div = new_binary(ND_DIV, node, assign());
        node = new_binary(ND_ASSIGN, node, div);
    }

    if (consume("+=")) {
        Node *add = new_binary(ND_ADD, node, ptr_calc(node, assign()));
        node = new_binary(ND_ASSIGN, node, add);
    }

    if (consume("-=")) {
        Node *sub = new_binary(ND_SUB, node, ptr_calc(node, assign()));
        node = new_binary(ND_ASSIGN, node, sub);
    }

    return node;
}

Node *conditional() {
    // a = b ? c : d;
    Node *node = logic_or();
    if (!consume("?")) {
        return node;
    }

    Node *ternary = new_node(ND_TERNARY);
    ternary->lhs = node; // b

    Node *ternary_right = new_node(ND_TERNARY_RIGHT);
    ternary_right->lhs = expr(); // c
    expect(":");
    ternary_right->rhs = conditional(); // d
    ternary->rhs = ternary_right;
    return ternary;
}

Node *logic_or() {
    Node *node = logic_and();
    while (consume("||")) {
        node = new_binary(ND_LOGICOR, node, logic_and());
    }
    return node;
}

Node *logic_and() {
    Node *node = bit_or();
    while (consume("&&")) {
        node = new_binary(ND_LOGICAND, node, bit_or());
    }
    return node;
}

Node *bit_or() {
    Node *node = bit_xor();
    while (consume("|")) {
        node = new_binary(ND_BITOR, node, bit_xor());
    }
    return node;
}

Node *bit_xor() {
    Node *node = bit_and();
    while (consume("^")) {
        node = new_binary(ND_BITXOR, node, bit_and());
    }
    return node;
}

Node *bit_and() {
    Node *node = equality();
    while (consume("&")) {
        node = new_binary(ND_BITAND, node, equality());
    }
    return node;
}

Node *equality() {
    Node *node = relational();
    for (;;) {
        if (consume("==")) {
            node = new_binary(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_binary(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

Node *relational() {
    Node *node = add();
    for (;;) {
        if (consume("<")) {
            node = new_binary(ND_LT, node, add());
        } else if (consume("<=")) {
            node = new_binary(ND_LE, node, add());
        } else if (consume(">")) {
            node = new_binary(ND_LT, add(), node);
        } else if (consume(">=")) {
            node = new_binary(ND_LE, add(), node);
        } else {
            return node;
        }
    }
}

Node *add() {
    Node *node = mul();
    for (;;) {
        if (consume("+")) {
            // ポインタの演算の場合は、ポインタのサイズ分を足す
            node = new_binary(ND_ADD, node, ptr_calc(node, mul()));
        } else if (consume("-")) {
            // ポインタの演算の場合は、ポインタのサイズ分を引く
            node = new_binary(ND_SUB, node, ptr_calc(node, mul()));
        } else {
            return node;
        }
    }
}

Node *ptr_calc(Node *node, Node *right) {
    if (node->type && node->type->ptr_to) {
        int n = node->type->ptr_to->ty == INT ? 4 
            : node->type->ptr_to->ty == CHAR ? 1
            : 8;
        return new_binary(ND_MUL, right, new_node_num(n));
    }
    return right;
}

Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume("*")) {
            node = new_binary(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_binary(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

Node *unary() {
    if (consume("+")) {
        return unary(); // +の場合は無視するということ
    }
    if (consume("-")) {
        return new_binary(ND_SUB, new_node_num(0), unary());
    }
    if (consume("*")) {
        return new_binary(ND_DEREF, unary(), NULL);
    }
    if (consume("&")) {
        return new_binary(ND_ADDR, unary(), NULL);
    }

    if (consume("!")) {
        return new_binary(ND_NOT, unary(), NULL);
    }

    if (consume("~")) {
        return new_binary(ND_BITNOT, unary(), NULL);
    }

    if (consume("++")) { // PRE INCREMENT
        Node *node = unary();
        Node *add = new_binary(ND_ADD, node, new_node_num(1));
        return new_binary(ND_ASSIGN, node, add);
    }
    if (consume("--")) { // PRE DECREMENT
        Node *node = unary();
        Node *sub = new_binary(ND_SUB, node, new_node_num(1));
        return new_binary(ND_ASSIGN, node, sub);
    }

    if (consume_kind(TK_SIZEOF)) {
        Token *current_tok = token; // ( の前のトークンを保存
        expect("(");

        Type *t = read_type();

        if (!t) { // sizeof(x)のxがintやcharなどではない場合
            token = current_tok; // トークンを元に戻す
            Node *n = unary(); // sizeof(x)のxをunaryで取得
            t = get_type(n);
        } else {
            expect(")");
        }

        int size = get_byte_size(t);
        return new_node_num(size);
    }

    return primary();
}

Node *primary() {
    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_kind(TK_IDENT);
    if (tok) {
        if (consume("(")) {
            // 関数呼び出し
            Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
            node->kind = ND_FUNC_CALL;
            node->func_name = static_cast<char*>(calloc(1,sizeof(char)));
            memcpy(node->func_name, tok->str, tok->len);

            // 引数 とりあえず10個まで
            node->block = static_cast<Node**>(calloc(10,sizeof(Node)));
            for (int i = 0; !consume(")"); i++) {
                node->block[i] = expr();
                if (consume(")")) {
                    break;
                }
                expect(",");
            }
            return node;
        }

        // enumの場合
        Node *num_node = find_enum_variable(tok);
        if (num_node) {
            return num_node;
        }

        // 関数呼び出しではない場合、変数。
        return variable(tok);
    }

    if (tok = consume_kind(TK_STRING)) {
        StringToken *s = static_cast<StringToken*>(calloc(1,sizeof(StringToken)));
        s->value = static_cast<char*>(calloc(100,sizeof(char)));
        memcpy(s->value, tok->str, tok->len);
        if (strings) {
            s->index = strings->index + 1;
        } else {
            s->index = 0;
        }
        s->next = strings;
        strings = s;

        return new_node_string(s);
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

Type *get_type(Node *node) {
    if (!node) {
        return NULL;
    }
    
    if (node->type) {
        return node->type;
    }

    Type *t = get_type(node->lhs);
    if (!t) {
        t = get_type(node->rhs);
    }

    if (t && node->kind == ND_DEREF) {
        t = t->ptr_to;
        if (t == NULL) {
            error("invalid pointer dereference.");
        }
        return t;
    }

    return t;
}

Type *define_struct() {
    if (!consume_kind(TK_STRUCT)) {
        return NULL;
    }

    Token *ident = consume_kind(TK_IDENT);
    if (ident && !peek_token_str("{")) {
        StructTag *tag = find_tag("struct", ident);
        if (!tag) {
            error("type not found.");
        }
        return tag->type;
    }

    expect("{");
    Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
    t->ty = STRUCT;
    int offset = 0;
    int max_size = 0;

    while (!consume("}")) {
        DefineFuncOrVariable *def_first_half = read_define_first_half();
        read_array_type_suffix(def_first_half);
        expect(";");

        Member *m = static_cast<Member*>(calloc(1,sizeof(Member)));
        m->name = static_cast<char*>(calloc(100,sizeof(char)));
        memcpy(m->name, def_first_half->ident->str, def_first_half->ident->len);
        m->type = def_first_half->type;
        int size = get_byte_size(def_first_half->type);
        offset = align_to(offset, size);
        m->offset = offset;
        offset += size;
        m->next = t->member_list;
        t->member_list = m;

        if (max_size <= 8 && max_size < size) {
            max_size = size;
        }
    }
    t->byte_size = offset;

    if (ident) {
        push_struct_tag_to_global("struct", ident, t, false); 
    }

    return t;
}

// 関数や変数定義の型を読み取る
// int *foo; int *foo() {} があった場合、int *の部分までを読む
Type *read_type() {
    Type *type = NULL;
    Token *t = token;

    // typedefの定義を探す
    Token *ident = consume_kind(TK_IDENT);
    if (ident) {
        StructTag *tag = find_tag(NULL,ident);
        if (!tag->type->is_imcomplete) {
            type = tag->type;
        } else {
            token = t;
        }
    }

    // structの定義を探す
    if (!type) {
        type = define_struct();
    }

    // enumの定義を探す
    if (!type) {
        type = define_enum();
    }

    // intやcharなどの型を探す
    if (!type) {
        Token *type_token = consume_kind(TK_TYPE);
        if (!type_token) {
            return NULL;
        }

        type = static_cast<Type*>(calloc(1,sizeof(Type)));
        bool is_char = memcmp("char", type_token->str, type_token->len) == 0;
        type->ty = is_char ? CHAR : INT; // 暫定void,bool,size_tはintのエイリアス
        type->ptr_to = NULL;
    }

    // derefの*を読む
    while (consume("*")) {
        Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
        t->ty = PTR;
        t->ptr_to = type;
        type = t; // 最終的にtypeは*intのような形になる
    }

    return type;
}

// 関数か変数定義の前半部分を読んで、それを返す
// int *foo; int *foo() {} があった場合、int *fooの部分までを読む
DefineFuncOrVariable *read_define_first_half() {
    Type *type = read_type(); // int *の部分を読む
    if (!type) {
        return NULL;
    }

    Token *tok = consume_kind(TK_IDENT);
    if (tok == NULL) {
        error("invalid define function or variable.");
    }

    DefineFuncOrVariable *def_first_half = static_cast<DefineFuncOrVariable*>(calloc(1,sizeof(DefineFuncOrVariable)));
    def_first_half->type = type;
    def_first_half->ident = tok;

    return def_first_half;
}

Node *initialize_local_variable(Node *node) {
    if (!node->variable->init_value) {
        return node;
    }

    // int x[] = {1,2,3} のような場合
    Node *assign_arr;

    if (node->type->ty == ARRAY && node->variable->init_value->block) {
        Node *block_node = static_cast<Node*>(calloc(1,sizeof(Node)));
        block_node->block = static_cast<Node**>(calloc(100,sizeof(Node)));
        block_node->kind = ND_BLOCK;

        for (int i = 0; node->variable->init_value->block[i]; i++) {
            // add = a[0]
            Node *add = static_cast<Node*>(calloc(1,sizeof(Node)));
            add->kind = ND_ADD;
            add->lhs = node;
            if (node->type && node->type->ty != INT) {
                int n = node->type->ptr_to->ty == INT ? 4 
                        : node->type->ptr_to->ty == CHAR ? 1
                        : 8;
                add->rhs = new_node_num(n * i); // n*iは配列の要素のサイズ
            }
            Node *deref = static_cast<Node*>(calloc(1,sizeof(Node)));
            deref->kind = ND_DEREF;
            deref->lhs = add;

            // = {1,2,3}の1や2や3の部分
            assign_arr = static_cast<Node*>(calloc(1,sizeof(Node)));
            assign_arr->kind = ND_ASSIGN;
            assign_arr->lhs = deref;
            assign_arr->rhs = node->variable->init_value->block[i];

            block_node->block[i] = assign_arr;
        }
        return block_node;
    }

    // arr[] = "foo"のような場合
    // これはarr[] = {'f','o','o','\0'}と同じｽ
    if (node->variable->init_value->kind == ND_STRING) {
        Node *block_node = static_cast<Node*>(calloc(1,sizeof(Node)));
        block_node->block = static_cast<Node**>(calloc(100,sizeof(Node)));
        block_node->kind = ND_BLOCK;

        int len = strlen(node->variable->init_value->string->value) + 1;

        for (int i = 0; i < node->type->array_size; i++) {
            // add = a[0]
            Node *add = static_cast<Node*>(calloc(1,sizeof(Node)));
            add->kind = ND_ADD;
            add->lhs = node;
            if (node->type && node->type->ty != INT) {
                int n = node->type->ptr_to->ty == INT ? 4 
                        : node->type->ptr_to->ty == CHAR ? 1
                        : 8;
                add->rhs = new_node_num(n * i); // n*iは配列の要素のサイズ
            }
            Node *deref = static_cast<Node*>(calloc(1,sizeof(Node)));
            deref->kind = ND_DEREF;
            deref->lhs = add;

            // = {1,2,3}の1や2や3の部分
            assign_arr = static_cast<Node*>(calloc(1,sizeof(Node)));
            assign_arr->kind = ND_ASSIGN;
            assign_arr->lhs = deref;
            assign_arr->rhs = 
                node->variable->init_value->string->value[i] == '\0' 
                ? new_node_num(0) 
                : new_node_num(node->variable->init_value->string->value[i]);

            block_node->block[i] = assign_arr;
        }
        return block_node;
    }

    // int a = 10; の a = 10を作る
    // aはnodeに入っていて, 10はnode->variable->init_valueに入っている
    Node *assign = static_cast<Node*>(calloc(1,sizeof(Node)));
    assign->kind = ND_ASSIGN;
    assign->lhs = node;
    assign->rhs = node->variable->init_value;
    return assign;
}

// 与えられた型が配列型である場合、その配列のサイズも読み取って型情報に保存する
// 配列型ではなかった場合は何もしない
void read_array_type_suffix(DefineFuncOrVariable *def_first_half) {
    if (!def_first_half) {
        error("read_array_type_suffix error. Invalid def_first_half variable.");
    }

    Type *type = def_first_half->type;

    // 配列かチェック
    while (consume("[")) {
        Type *t = static_cast<Type*>(calloc(1,sizeof(Type))); // 新しい型情報を作る
        t->ty = ARRAY;
        t->ptr_to = type; // 元の型情報をtのポインタに設定する
        t->array_size = 0;

        Token *array_num = NULL;
        // 配列のサイズが指定されている場合
        if (array_num = consume_kind(TK_NUM)) {
            t->array_size = array_num->val;
        }

        type = t;
        expect("]");
    }
    def_first_half->type = type;
}

int get_byte_size(Type *type) {
    // sizeof(数値)の場合は4を返しておく
    if (type == NULL) {
        return 4;
    }

    if (type->ty == STRUCT) {
        return type->byte_size;
    }

    if (type->ty == ARRAY) {
        if (type->array_size == 0) {
            error("array size is not specified.");
        }
        return get_byte_size(type->ptr_to) * type->array_size;
    }

    return type->ty == PTR ? 8 : type->ty == CHAR ? 1 : 4;
}

// まだ定義されていない変数の定義を行う
// int *foo; int *foo() {} などがあった場合、int *fooの部分までがdef_first_half
Node *define_variable(DefineFuncOrVariable *def_first_half, Variable **variable_list) {
    read_array_type_suffix(def_first_half);
    Type *type = def_first_half->type;

    // 初期化式
    Node *init_value = NULL;
    if (consume("=")) {
        if (consume("{")) {
            // 配列の初期化
            // int a[3] = {1,2,3} のような場合
            init_value = static_cast<Node*>(calloc(1,sizeof(Node)));
            init_value->block = static_cast<Node**>(calloc(10,sizeof(Node)));
            int i;
            for (i = 0; !consume("}"); i++) {
                init_value->block[i] = expr();
                if (consume("}")) {
                    break;
                }
                expect(",");
            }
            if (type->array_size < i) { // arr[] = {1,2} のような場合
                type->array_size = i + 1;
            }
            for (i = i + 1; i < type->array_size; i++) { // arr[5] = {1,2} のような場合
                init_value->block[i] = new_node_num(0);
            }
        } else {
            // 定数式の場合
            // int a = 3; のような場合
            init_value = expr();

            // stringの場合
            // char arr[] = "abc"; のような場合
            if (init_value->kind == ND_STRING) {
                int len = strlen(init_value->string->value) + 1;
                if (type->array_size < len) {
                    type->array_size = len;
                }
            }
        }
    }

    Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
    node->variable_name = static_cast<char*>(calloc(100,sizeof(char)));
    memcpy(node->variable_name, def_first_half->ident->str, def_first_half->ident->len);
    node->byte_size = get_byte_size(type);

    Variable *local_variable = find_varable(def_first_half->ident);
    if (local_variable != NULL) {
        // コンパイルのため。副作用あるかも。ブロックスコープを実装していないため
        // error2("redefined variable: %s", node->variable_name);
    }

    // FIXME
    if (locals == variable_list) {
        node->kind = ND_LOCAL_VARIABLE;
    } else {
        node->kind = ND_GLOBAL_VARIABLE_USE;
    }

    // ローカル変数の場合は、関数ごとのindex, グローバル変数の場合は0番目のindex
    int current_index = locals == variable_list ? current_func : 0;

    local_variable = static_cast<Variable*>(calloc(1,sizeof(Variable)));
    local_variable->next = variable_list[current_index];
    local_variable->name = def_first_half->ident->str;
    local_variable->len = def_first_half->ident->len;
    local_variable->init_value = init_value;
    if (variable_list[current_index] == NULL) {
        local_variable->offset = 8;
    } else {
        local_variable->offset = variable_list[current_index]->offset + node->byte_size;
    }
    local_variable->type = type;

    node->offset = local_variable->offset;
    node->type = local_variable->type;
    node->variable = local_variable;
    variable_list[current_index] = local_variable;
    char name[100] = {0};
    memcpy(name, def_first_half->ident->str, def_first_half->ident->len);
    // fprintf(stderr, "*NEW VARIABLE* %s\n", name);
    return node;
}

// 定義済みの変数を参照する
Node *variable(Token *tok) {
    Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
    node->variable_name = static_cast<char*>(calloc(100,sizeof(char)));
    memcpy(node->variable_name, tok->str, tok->len);

    Variable *local_variable = find_varable(tok);
    if (local_variable == NULL) {
        error2("undefined variable: %s", node->variable_name);
    }

    if (local_variable->kind == Variable::LOCAL_VARIABLE) {
        node->kind = ND_LOCAL_VARIABLE;
    } else {
        node->kind = ND_GLOBAL_VARIABLE_USE;
    }
    node->offset = local_variable->offset;
    node->type = local_variable->type;

    char *node_varname = node->variable_name;

    while (true) {
        // a[3] は *(a + 3) と同じ  tokでaまで取れている
        if (consume("[")) {
            // nodeは現在a 
            // addのlhsにa, rhsに3*4を入れる(4はaがintの場合のサイズ)
            // addには(a + 3)が入る
            Node *add = static_cast<Node*>(calloc(1,sizeof(Node)));
            add->kind = ND_ADD;
            add->lhs = node;
            if (node->type && node->type->ty != INT) {
                int n = node->type->ptr_to->ty == INT ? 4 
                        : node->type->ptr_to->ty == CHAR ? 1
                        : 8;
                // 型のサイズにexpr()の値をかけた数字をrhsに入れる
                add->rhs = new_binary(ND_MUL, expr(), new_node_num(n));
            }

            // nodeのtypeを取っておく
            Type *node_type = node->type;

            // 新しいnodeを作って、lhsに(a + 3)のaddを入れる
            // 最終的にnodeを返すため、nodeを新しく更新している
            node = static_cast<Node*>(calloc(1,sizeof(Node)));
            node->kind = ND_DEREF;
            node->lhs = add;
            node->type = node_type->ptr_to;
            node->variable_name = node_varname;

            expect("]");
            continue;
        }

        if (consume(".")) {
            node = struct_reference(node);
            continue;
        }

        if (consume("->")) {
            // x->y は (*x).y と同じ
            Type *t = node->type->ptr_to;
            node = new_binary(ND_DEREF, node, NULL);
            node->type = t;
            node = struct_reference(node);
            continue;
        }

        if (consume("++")) { // POST INCREMENT
            // (a+=1)-1
            Node *add = new_binary(ND_ADD, node, new_node_num(1));
            node = new_binary(ND_ASSIGN, node, add);
            // assignの後にsubを入れてもaの値は変わらず、その行はaに1引いた値が評価値となる
            node = new_binary(ND_SUB, node, new_node_num(1)); 
            continue;
        }

        if (consume("--")) { // POST DECREMENT
            // (a-=1)+1
            Node *sub = new_binary(ND_SUB, node, new_node_num(1));
            node = new_binary(ND_ASSIGN, node, sub);
            // assignの後にaddを入れてもaの値は変わらず、その行はaに1足した値が評価値となる
            node = new_binary(ND_ADD, node, new_node_num(1));
            continue;
        }

        break;
    }
    return node;
}

Node *struct_reference(Node *node) {
    Node *member_node = static_cast<Node*>(calloc(1,sizeof(Node)));
    member_node->kind = ND_MEMBER;
    member_node->lhs = node;
    member_node->member = find_member(consume_kind(TK_IDENT), node->type);
    member_node->type = member_node->member->type;
    return member_node;
}

Member *find_member(Token *tok, Type *type) {
    if (!tok) {
        error("invalid member.");
    }

    if (!type) {
        error("invalid member.");
    }
    char token_str[100] = {0};
    memcpy(token_str, tok->str, tok->len);
    for (Member *m = type->member_list; m; m = m->next) {
        if (memcmp(m->name, token_str, tok->len) == 0) {
            return m;
        }
    }
    error("undefined member");
    return NULL;
}

// 変数を名前で検索する。
Variable *find_varable(Token *tok) {
    // まずローカルの変数を検索
    for (Variable *var = locals[current_func]; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            var->kind = Variable::LOCAL_VARIABLE;
            return var;
        }
    }

    // ローカル変数に変数名が無ければ次にグローバル変数を検索
    for (Variable *var = globals[0]; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            var->kind = Variable::GLOBAL_VARIABLE;
            return var;
        }
    }

    // どちらもなければNULLを返すｽ
    return NULL;
}

// アライメントを揃える
// struct {int a; char b1; char b2; int c;}
// a 4byte
// b1 1byte
// b2 1byte
//    2byte <- 4byteになるようにアライメントを揃える
// c 4byte
// align_to(12, 8) = 16
// byte_sizeを最も近いalignの倍数に切り上げる
int align_to(int byte_size, int align) {
    return (byte_size + align - 1) & ~(align - 1); // 2のべき乗でアライメントを揃える
}

void push_struct_tag_to_global(const char* prefix, Token *tok, Type *type, bool is_typedef) {
    StructTag *tag = find_tag(prefix, tok);

    char *tag_name = static_cast<char*>(calloc(100,sizeof(char)));
    if (prefix) {
        memcpy(tag_name, prefix, strlen(prefix));
        memcpy(tag_name + strlen(prefix), " ", 1);
        memcpy(tag_name + strlen(prefix) + 1, tok->str, tok->len);
    } else {
        memcpy(tag_name, tok->str, tok->len);
    }

    if (is_typedef) {
        tag->type = type;
        tag->type->is_imcomplete = false; // なぜかフラグが立っているので落とす
    } else {
        // tag->typeを直接更新すると、すでにtag->typeを参照しているところが
        // imcompleteなtypeを参照してしまうので、直接更新しない
        tag->type->array_size = type->array_size;
        tag->type->is_imcomplete = false;
        tag->type->member_list = type->member_list;
        tag->type->ptr_to = type->ptr_to;
        tag->type->byte_size = type->byte_size;
        tag->type->ty = type->ty;
    }

}

StructTag *find_tag(const char* prefix, Token *tok) {
    char *tag_name = static_cast<char*>(calloc(100,sizeof(char)));
    if (prefix) {
        memcpy(tag_name, prefix, strlen(prefix));
        memcpy(tag_name + strlen(prefix), " ", 1);
        memcpy(tag_name + strlen(prefix) + 1, tok->str, tok->len);
    } else {
        memcpy(tag_name, tok->str, tok->len);
    }

    for (StructTag *tag = struct_tags; tag; tag = tag->next) {
        if (strcmp(tag->name, tag_name) == 0) {
            return tag;
        }
    }

    // 不完全な状態で型を作っておく
    StructTag *tag = static_cast<StructTag*>(calloc(1,sizeof(StructTag)));
    tag->name = tag_name;
    tag->type = static_cast<Type*>(calloc(1,sizeof(Type)));
    tag->type->is_imcomplete = true;
    if (struct_tags) {
        tag->next = struct_tags;
    }
    struct_tags = tag;
    return tag;
}

bool define_typedef() {
    if (!consume_kind(TK_TYPEDEF)) {
        return false;
    }
    DefineFuncOrVariable *def_first_half = read_define_first_half();
    expect(";");
    push_struct_tag_to_global(NULL, def_first_half->ident, def_first_half->type, true);
    return true;
}

Type *define_enum() {
    if (!consume_kind(TK_ENUM)) {
        return NULL;
    }

    Token *ident = consume_kind(TK_IDENT);
    if (ident && !peek_token_str("{")) {
        StructTag *tag = find_tag("enum", ident);
        if (!tag) {
            error("enum ident not found.");
        }
        return tag->type;
    }

    expect("{");
    int enum_index = 0;
    while (true) {
        // 最後の要素にはカンマがついてても良い
        if (consume("}")) {
            break;
        }

        Token *tok = consume_kind(TK_IDENT);

        if (consume("=")) {
            enum_index = expect_number();
        } else {
            enum_index++;
        }

        EnumVariable *e = static_cast<EnumVariable*>(calloc(1,sizeof(EnumVariable)));
        e->name = static_cast<char*>(calloc(100,sizeof(char)));
        memcpy(e->name, tok->str, tok->len);
        e->value = enum_index;
        e->next = enum_variables;
        enum_variables = e;

        if (consume("}")) {
            break;
        }
        expect(",");
    }

    if (ident) {
        push_struct_tag_to_global("enum", ident, int_type(), false);
    }
    return int_type();
}

Type *int_type() {
    Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
    t->ty = INT;
    t->byte_size = 4;
    return t;
}

Node *find_enum_variable(Token *tok) {
    char token_str[100] = {0};
    memcpy(token_str, tok->str, tok->len);

    for (EnumVariable *e = enum_variables; e; e = e->next) {
        if (strcmp(e->name, token_str) == 0) {
            return new_node_num(e->value);
        }
    }

    return NULL;
}