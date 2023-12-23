#include "9cc.h"

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

Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

//100行までしか対応していない
Node *code[100];

// 9cc.hに構文あり
void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = func();
    }
    code[i] = NULL;
}

Node *func() {
    currentFunc++;
    Node *node;

    if (!consume_kind(TK_TYPE)) {
        error("function return type not found.");
    }

    Token *tok = consume_kind(TK_IDENT);
    if (tok == NULL) {
        error("not function!");
    }

    node = static_cast<Node*>(calloc(1,sizeof(Node)));
    node->kind = ND_FUNC_DEF;
    node->funcName = static_cast<char*>(calloc(1,sizeof(char)));
    memcpy(node->funcName, tok->str, tok->len);
    node->funcArgs = static_cast<Node**>(calloc(10,sizeof(Node*))); //引数10個分の配列の長さを作る
    
    expect("(");
    for (int i = 0; !consume(")"); i++) {
        if (!consume_kind(TK_TYPE)) {
            error("function args type not found.");
        }

        node->funcArgs[i] = define_variable();

        if (consume(")")) {
            break;
        }
        expect(",");
    }

    node->lhs = stmt();
    return node;
}

Node *stmt() {
    Node *node;

    if (consume("{")) {
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_BLOCK;
        //100行までしか対応していない
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
            left->lhs = expr();
            expect(";");
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
        node->lhs = expr();
        expect(";");
        return node;
    }

    if (consume_kind(TK_TYPE)) {
        node = define_variable();
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
    Node *node = equality();
    if (consume("=")) {
        node = new_binary(ND_ASSIGN, node, assign());
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
            Node *r = mul();

            // ポインタの演算の場合は、ポインタのサイズ分を足す
            if (node->type && node->type->ty == Type::PTR) {
                int n = node->type->ptr_to->ty == Type::INT ? 4 : 8;
                r = new_binary(ND_MUL, r, new_node_num(n));
            }

            node = new_binary(ND_ADD, node, r);
        } else if (consume("-")) {
            Node *r = mul();
            
            // ポインタの演算の場合は、ポインタのサイズ分を引く
            if (node->type && node->type->ty == Type::PTR) {
                int n = node->type->ptr_to->ty == Type::INT ? 4 : 8;
                r = new_binary(ND_MUL, r, new_node_num(n));
            }

            node = new_binary(ND_SUB, node, r);
        } else {
            return node;
        }
    }
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
        return unary(); //+の場合は無視するということ
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
    if (consume_kind(TK_SIZEOF)) {
        Node *node = unary();
        int size = node->type && node->type->ty == Type::PTR ? 8 : 4;
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
            //関数呼び出し
            Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
            node->kind = ND_FUNC_CALL;
            node->funcName = static_cast<char*>(calloc(1,sizeof(char)));
            memcpy(node->funcName, tok->str, tok->len);

            //引数 とりあえず10個まで
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

        //関数呼び出しではない場合、変数。
        return variable(tok);
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

// まだ定義されていない変数の定義を行う
Node *define_variable() {
    Type *type = static_cast<Type*>(calloc(1,sizeof(Type)));
    type->ty = Type::INT;
    type->ptr_to = NULL;
    while (consume("*")) {
        Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
        t->ty = Type::PTR;
        t->ptr_to = type;
        type = t;
    }

    Token *tok = consume_kind(TK_IDENT);
    if (tok == NULL) {
        error("invalid define variable.");
    }

    int size = type->ty == Type::PTR ? 8 : 4;

    // 配列かチェック
    while (consume("[")) {
        Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
        t->ty = Type::ARRAY;
        t->ptr_to = type;
        t->array_size = expect_number();
        type = t;
        size *= t->array_size;
        expect("]");
    }

    // sizeを8の倍数にする
    while ((size % 8) != 0) {
        size += 4;
    }

    Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
    node->kind = ND_LOCAL_VARIABLE;

    LocalVariable *local_variable = find_local_variable(tok);
    if (local_variable != NULL) {
        char name[100] = {0};
        memcpy(name, tok->str, tok->len);
        error("redefined variable: %s", name);
    }

    local_variable = static_cast<LocalVariable*>(calloc(1,sizeof(LocalVariable)));
    local_variable->next = locals[currentFunc];
    local_variable->name = tok->str;
    local_variable->len = tok->len;
    if (locals[currentFunc] == NULL) {
        local_variable->offset = 8;
    } else {
        local_variable->offset = locals[currentFunc]->offset + size;
    }
    local_variable->type = type;

    node->offset = local_variable->offset;
    node->type = local_variable->type;
    locals[currentFunc] = local_variable;
    char name[100] = {0};
    memcpy(name, tok->str, tok->len);
    // fprintf(stderr, "*NEW VARIABLE* %s\n", name);
    return node;
}

// 定義済みの変数を参照する
Node *variable(Token *tok) {
    Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
    node->kind = ND_LOCAL_VARIABLE;

    LocalVariable *local_variable = find_local_variable(tok);
    if (local_variable == NULL) {
        char name[100] = {0};
        memcpy(name, tok->str, tok->len);
        error("undefined variable: %s", name);
    }

    node->offset = local_variable->offset;
    node->type = local_variable->type;
    return node;
}