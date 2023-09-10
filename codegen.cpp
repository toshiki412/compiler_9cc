#include "9cc.h"

Node *new_node(NodeKind kind){
    Node *node = static_cast<Node*>(calloc(1, sizeof(Node)));
    node->kind = kind;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs){
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs; 
    return node;
}

Node *new_node_num(int val){
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

Node *code[100];

void program(){
    int i = 0;
    while(!at_eof()){
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *stmt(){
    Node *node;

    if(consume_kind(TK_FOR)){
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_FOR;

        Node *left = static_cast<Node*>(calloc(1,sizeof(Node)));
        left->kind = ND_FOR_LEFT;
        Node *right = static_cast<Node*>(calloc(1,sizeof(Node)));
        right->kind = ND_FOR_RIGHT;

        if(!consume(";")){
            left->lhs = expr();
            expect(";");
        }
        if(!consume(";")){
            left->rhs = expr();
            expect(";");
        }

        if(!consume(")")){
            right->lhs = expr();
            expect(")");
        }
        right->rhs = stmt();

        node->lhs = left;
        node->rhs = right;
        return node;
    }

    if(consume_kind(TK_WHILE)){
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_WHILE;
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        return node;
    }

    if(consume_kind(TK_IF)){
        expect("(");
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_IF;
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        if(consume_kind(TK_ELSE)){
            Node *els = static_cast<Node*>(calloc(1,sizeof(Node)));
            els->kind = ND_ELSE;
            els->lhs = node->rhs;
            els->rhs = stmt();
            node->rhs = els;
        }
        return node;
    }

    if(consume_kind(TK_RETURN)){
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_RETURN;
        node->lhs = expr();
    }else{
        node = expr();
    }

    expect(";");
    return node;
}

Node *expr(){
    return assign();
}

Node *assign(){
    Node *node = equality();
    if(consume("=")){
        node = new_binary(ND_ASSIGN, node, assign());
    }
    return node;
}

Node *equality(){
    Node *node = relational();
    for(;;){
        if(consume("==")){
            node = new_binary(ND_EQ, node, relational());
        }else if(consume("!=")){
            node = new_binary(ND_NE, node, relational());
        }else{
            return node;
        }
    }
}

Node *relational(){
    Node *node = add();
    for(;;){
        if(consume("<")){
            node = new_binary(ND_LT, node, add());
        }else if(consume("<=")){
            node = new_binary(ND_LE, node, add());
        }else if(consume(">")){
            node = new_binary(ND_LT, add(), node);
        }else if(consume(">=")){
            node = new_binary(ND_LE, add(), node);
        }else{
            return node;
        }
    }
}

Node *add(){
    Node *node = mul();
    for(;;){
        if(consume("+")){
            node = new_binary(ND_ADD, node, mul());
        }else if(consume("-")){
            node = new_binary(ND_SUB, node, mul());
        }else{
            return node;
        }
    }
}

Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume("*")){
            node = new_binary(ND_MUL, node, unary());
        }else if (consume("/")){
            node = new_binary(ND_DIV, node, unary());
        }else{
            return node;
        }
    }
}

Node *unary(){
    if(consume("+")){
        return unary(); //+の場合は無視するということ
    }
    if(consume("-")){
        return new_binary(ND_SUB, new_node_num(0), unary());
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
    if(tok){
        Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if(lvar){
            node->offset = lvar->offset;
        }else{
            lvar = static_cast<Lvar*>(calloc(1,sizeof(LVar)));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            if(locals == NULL){
                lvar->offset = 8;
            }else{
                lvar->offset = locals->offset + 8;
            }
            node->offset = lvar->offset;
            locals = lvar;
        }
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

void gen_lval(Node *node){
    if(node->kind != ND_LVAR){
        error("not ND_LVAR");
    }

    printf(" mov rax, rbp\n");
    printf(" sub rax, %d\n", node->offset);
    printf(" push rax\n");
}

int genCounter = 0;

//スタックマシン
void gen(Node *node){
    if(!node) return;
    genCounter += 1;
    int id = genCounter;

    switch (node->kind){
    case ND_FOR:
        //for(A;B;C) D;
        gen(node->lhs->lhs);        //Aをコンパイルしたコード
        printf(".Lbegin%03d:\n", id);
        gen(node->lhs->rhs);        //Bをコンパイルしたコード
        if(!node->lhs->rhs){        //無限ループの対応
            printf(" push 1\n");
        }
        printf(" pop rax\n");
        printf(" cmp rax, 0\n");
        printf(" je .Lend%03d\n", id);
        gen(node->rhs->rhs);        //Dをコンパイルしたコード
        gen(node->rhs->lhs);        //Cをコンパイルしたコード 
        printf(" jmp .Lbegin%03d\n", id);
        printf(".Lend%03d:\n", id);
        return;
    case ND_WHILE:
        //while(A) B;
        printf(".Lbegin%03d:\n", id);
        gen(node->lhs);             //Aをコンパイルしたコード
        printf(" pop rax\n");
        printf(" cmp rax, 0\n");
        printf(" je .Lend%03d\n", id);
        gen(node->rhs);             //Bをコンパイルしたコード
        printf(" jmp .Lbegin%03d\n", id);
        printf(".Lend%03d:\n", id);
        return;
    case ND_IF:
        //if(A) B; else C;
        gen(node->lhs);             //Aをコンパイルしたコード
        printf(" pop rax\n");
        printf(" cmp rax, 0\n");
        printf(" je .Lelse%03d\n", id);
        if(node->rhs->kind == ND_ELSE){
            gen(node->rhs->lhs);    //Bをコンパイルしたコード
        }else{
            gen(node->rhs);         //Bをコンパイルしたコード
        }
        printf(" jmp .Lend%03d\n", id);
        printf(".Lelse%03d:\n", id);
        if(node->rhs->kind == ND_ELSE){
            gen(node->rhs->rhs);    //Cをコンパイルしたコード
        }
        printf(".Lend%03d:\n", id);
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf(" pop rax\n");
        printf(" mov rsp, rbp\n");
        printf(" pop rbp\n");
        printf(" ret\n");
        return;
    case ND_NUM:
        printf(" push %d\n", node->val);
        return;
    case ND_LVAR:
        gen_lval(node);
        printf(" pop rax\n");
        printf(" mov rax, [rax]\n");
        printf(" push rax\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);
        printf(" pop rdi\n");
        printf(" pop rax\n");
        printf(" mov [rax], rdi\n");
        printf(" push rdi\n");
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
    case ND_EQ:
        printf(" cmp rax, rdi\n");
        printf(" sete al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_NE:
        printf(" cmp rax, rdi\n");
        printf(" setne al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_LT:
        printf(" cmp rax, rdi\n");
        printf(" setl al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_LE:
        printf(" cmp rax, rdi\n");
        printf(" setle al\n");
        printf(" movzb rax, al\n");
        break;
    }
    
    printf(" push rax\n");
}

