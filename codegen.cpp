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

void gen_left_value(Node *node) {
    // derefである場合 *p = 1; など
    if (node->kind == ND_DEREF) {
        gen(node->lhs);
        return;
    }

    if (node->kind != ND_LOCAL_VARIABLE) {
        error("not ND_LOCAL_VARIABLE");
    }

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

int genCounter = 0; //genが呼ばれるたびにインクリメントされる

//x86-64のABIに従い、引数はとりあえず6つまで
const char *argRegisters[] = {"rdi", "rsi", "rdx", "rcx", "r8","r9"}; 

//スタックマシン
void gen(Node *node) {
    if (!node) return;
    genCounter += 1;
    int labelId = genCounter;
    int functionArgNum = 0;

    switch (node->kind) {
    case ND_ADDR:
        gen_left_value(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_FUNC_DEF:
        printf("%s:\n", node->funcName);

        //プロローグ
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        //引数の値をスタックに積む
        for (int i = 0; node->funcArgs[i]; i++) {
            printf("    push %s\n", argRegisters[i]);
            functionArgNum++;
        }
        //引数の数を除いた変数の数だけrspをずらして、変数領域を確保する。
        if (locals[currentFunc]) {
            // 全体のずらすべき数
            int offset = locals[currentFunc][0].offset;

            for (LocalVariable *cur = locals[currentFunc]; cur; cur = cur->next) {
                // TODO 現在はint型のみ対応
                offset += cur->type->ty == Type::ARRAY ? cur->type->array_size * 4 : 8;
            }

            // スタックに積んだ引数の数を除いた数
            offset -= functionArgNum * 8;
            
            printf("    sub rsp, %d\n", offset);
        }

        gen(node->lhs);

        //エピローグ
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");

        return;
    case ND_FUNC_CALL:
        for (int i = 0; node->block[i]; i++) {
            gen(node->block[i]);
            functionArgNum++;
        }

        //スタックに積んだ引数の値を取り出す
        for (int i = functionArgNum - 1; i >= 0; i--) {
            printf("    pop %s\n", argRegisters[i]);
        }
        
        //関数呼び出し
        printf("    mov rax, rsp\n");
        printf("    and rax, 15\n");                //下位４bitをマスクする rspが16の倍数かどうかをチェックする
        printf("    jnz .L.call.%03d\n", labelId);  //16の倍数じゃないならばジャンプ
        printf("    mov rax, 0\n");
        printf("    call %s\n", node->funcName);    //関数呼び出し
        printf("    jmp .L.end.%03d\n", labelId);
        printf(".L.call.%03d:\n", labelId);
        printf("    sub rsp, 8\n");                 // rspを16の倍数にするために8バイト分ずらす
        printf("    mov rax, 0\n");
        printf("    call %s\n", node->funcName);    //関数呼び出し
        printf("    add rsp, 8\n");                 // 8バイトずらしたrspを元に戻す
        printf(".L.end.%03d:\n", labelId);
        printf("    push rax\n");                   //関数からリターンしたときにraxに入っている値が関数の返り値という約束

        return;
    case ND_BLOCK:
        // ND_BLOCKに含まれるステートメントのコードを順番に生成する
        for (int i = 0; node->block[i]; i++) {
            gen(node->block[i]);
            // printf("    pop rax\n");
        }
        return;
    case ND_FOR:
        //for (A;B;C) D;
        gen(node->lhs->lhs);        //Aをコンパイルしたコード
        printf(".Lbegin%03d:\n", labelId);
        gen(node->lhs->rhs);        //Bをコンパイルしたコード
        if (!node->lhs->rhs) {        //無限ループの対応
            printf("    push 1\n");
        }
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%03d\n", labelId);
        gen(node->rhs->rhs);        //Dをコンパイルしたコード
        gen(node->rhs->lhs);        //Cをコンパイルしたコード 
        printf("    jmp .Lbegin%03d\n", labelId);
        printf(".Lend%03d:\n", labelId);
        return;
    case ND_WHILE:
        //while (A) B;
        printf(".Lbegin%03d:\n", labelId);
        gen(node->lhs);             //Aをコンパイルしたコード
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%03d\n", labelId);
        gen(node->rhs);             //Bをコンパイルしたコード
        printf("    jmp .Lbegin%03d\n", labelId);
        printf(".Lend%03d:\n", labelId);
        return;
    case ND_IF:
        //if (A) B; else C;
        gen(node->lhs);             //Aをコンパイルしたコード
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lelse%03d\n", labelId);
        if (node->rhs->kind == ND_ELSE) {
            gen(node->rhs->lhs);    //Bをコンパイルしたコード
        } else {
            gen(node->rhs);         //Bをコンパイルしたコード
        }
        printf("    jmp .Lend%03d\n", labelId);
        printf(".Lelse%03d:\n", labelId);
        if (node->rhs->kind == ND_ELSE) {
            gen(node->rhs->rhs);    //Cをコンパイルしたコード
        }
        printf(".Lend%03d:\n", labelId);
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_NUM:
        printf("    push %d\n", node->val);
        return;
    case ND_LOCAL_VARIABLE:
        gen_left_value(node);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_ASSIGN:
        gen_left_value(node->lhs);
        gen(node->rhs);
        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    case ND_EQ:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LT:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    }
    
    printf("    push rax\n");
}

