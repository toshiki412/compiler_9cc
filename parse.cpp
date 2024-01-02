#include "9cc.h"

//���[�J���ϐ� 100�̊֐��܂őΉ�
Variable *locals[100];

// �O���[�o���ϐ�
Variable *globals[100];

int current_func = 0;

StringToken *strings;

int struct_def_index = 0;
StructTag *struct_tags;

EnumVariable *enum_variables;

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

//100�s�܂ł����Ή����Ă��Ȃ�
Node *code[100];

// 9cc.h�ɍ\������
void program() {
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

    // int *foo() {} �̂悤�Ȋ֐���`�̏ꍇ, int *foo�̕�����ǂ�
    DefineFuncOrVariable *def_first_half = read_define_first_half();

    if (consume("(")) {
        current_func++;

        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_FUNC_DEF;
        node->func_name = static_cast<char*>(calloc(100,sizeof(char)));
        memcpy(node->func_name, def_first_half->ident->str, def_first_half->ident->len);
        node->func_args = static_cast<Node**>(calloc(10,sizeof(Node*))); //����10���̔z��̒��������

        for (int i = 0; !consume(")"); i++) {
            node->func_args[i] = define_variable(read_define_first_half(), locals);

            if (consume(")")) {
                break;
            }
            expect(",");
        }

        node->lhs = stmt();
        return node;
    } else {
        // foo�̂���( �łȂ���Εϐ���`�ł���    
        node = define_variable(def_first_half, globals); // �O���[�o���ϐ��̓o�^
        node->kind = ND_GLOBAL_VARIABLE_DEF; // �O���[�o���ϐ��̏ꍇ��ND_GLOBAL_VARIABLE_DEF�ɏ��������� ������Ɣ���
        expect(";");
        return node;
    }
}

Node *stmt() {
    Node *node;

    if (consume("{")) {
        node = static_cast<Node*>(calloc(1,sizeof(Node)));
        node->kind = ND_BLOCK;
        //100�s�܂ł����Ή����Ă��Ȃ�
        node->block = static_cast<Node**>(calloc(100,sizeof(Node)));
        for (int i = 0; !consume("}"); i++) {
            node->block[i] = stmt(); // {}���ɂ���stmt��ǉ�
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
    Node *node = equality();
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
        Node *add = new_binary(ND_ADD, node, assign());
        node = new_binary(ND_ASSIGN, node, add);
    }

    if (consume("-=")) {
        Node *sub = new_binary(ND_SUB, node, assign());
        node = new_binary(ND_ASSIGN, node, sub);
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

            // �|�C���^�̉��Z�̏ꍇ�́A�|�C���^�̃T�C�Y���𑫂�
            if (node->type && node->type->ty != INT) {
                int n = node->type->ptr_to->ty == INT ? 4 
                    : node->type->ptr_to->ty == CHAR ? 1
                    : 8;
                r = new_binary(ND_MUL, r, new_node_num(n));
            }

            node = new_binary(ND_ADD, node, r);
        } else if (consume("-")) {
            Node *r = mul();
            
            // �|�C���^�̉��Z�̏ꍇ�́A�|�C���^�̃T�C�Y��������
            if (node->type && node->type->ty != INT) {
                int n = node->type->ptr_to->ty == INT ? 4 
                    : node->type->ptr_to->ty == CHAR ? 1
                    : 8;
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
        return unary(); //+�̏ꍇ�͖�������Ƃ�������
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
        Node *n = unary();
        Type *t = get_type(n);
        int size = t && t->ty == PTR ? 8 
                : t && t->ty == CHAR ? 1
                : 4;
        return new_node_num(size);
    }
    return primary();
}

Node *primary() {
    // ���̃g�[�N����"("�Ȃ�A"(" expr ")"�̂͂�
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_kind(TK_IDENT);
    if (tok) {
        if (consume("(")) {
            //�֐��Ăяo��
            Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
            node->kind = ND_FUNC_CALL;
            node->func_name = static_cast<char*>(calloc(1,sizeof(char)));
            memcpy(node->func_name, tok->str, tok->len);

            //���� �Ƃ肠����10�܂�
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

        // enum�̏ꍇ
        Node *num_node = find_enum_variable(tok);
        if (num_node) {
            return num_node;
        }

        //�֐��Ăяo���ł͂Ȃ��ꍇ�A�ϐ��B
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

    // �����łȂ���ΐ��l�̂͂�
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
        read_type(def_first_half);
        expect(";");

        Member *m = static_cast<Member*>(calloc(1,sizeof(Member)));
        m->name = static_cast<char*>(calloc(100,sizeof(char)));
        memcpy(m->name, def_first_half->ident->str, def_first_half->ident->len);
        m->type = def_first_half->type;
        int size = get_size(def_first_half->type);
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
        push_struct_tag_to_global("struct", ident, t); 
    }

    return t;
}

// �֐����ϐ���`�̑O��������ǂ�ŁA�����Ԃ�
// int *foo; int *foo() {} ���������ꍇ�Aint *foo�̕����܂ł�ǂ�
DefineFuncOrVariable *read_define_first_half() {
    Type *type = NULL;
    Token *t = token;
    Token *ident = consume_kind(TK_IDENT);
    if (ident) {
        StructTag *tag = find_tag(NULL,ident);
        if (tag) {
            type = tag->type;
        } else {
            token = t;
        }
    }
    if (!type) {
        type = define_struct();
    }
    if (!type) {
        type = define_enum();
    }
    if (!type) {
        Token *type_token = consume_kind(TK_TYPE);
        if (!type_token) {
            return NULL;
        }

        type = static_cast<Type*>(calloc(1,sizeof(Type)));
        bool is_char = memcmp("char", type_token->str, type_token->len) == 0;
        type->ty = is_char ? CHAR : INT;
        type->ptr_to = NULL;
    }

    // deref��*��ǂ�
    while (consume("*")) {
        Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
        t->ty = PTR;
        t->ptr_to = type;
        type = t; // �ŏI�I��type��*int�̂悤�Ȍ`�ɂȂ�
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

    // int x[] = {1,2,3} �̂悤�ȏꍇ
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
                add->rhs = new_node_num(n * i); // n*i�͔z��̗v�f�̃T�C�Y
            }
            Node *deref = static_cast<Node*>(calloc(1,sizeof(Node)));
            deref->kind = ND_DEREF;
            deref->lhs = add;

            // = {1,2,3}��1��2��3�̕���
            assign_arr = static_cast<Node*>(calloc(1,sizeof(Node)));
            assign_arr->kind = ND_ASSIGN;
            assign_arr->lhs = deref;
            assign_arr->rhs = node->variable->init_value->block[i];

            block_node->block[i] = assign_arr;
        }
        return block_node;
    }

    // arr[] = "foo"�̂悤�ȏꍇ
    // �����arr[] = {'f','o','o','\0'}�Ɠ���
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
                add->rhs = new_node_num(n * i); // n*i�͔z��̗v�f�̃T�C�Y
            }
            Node *deref = static_cast<Node*>(calloc(1,sizeof(Node)));
            deref->kind = ND_DEREF;
            deref->lhs = add;

            // = {1,2,3}��1��2��3�̕���
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

    // int a = 10; �� a = 10�����
    // a��node�ɓ����Ă���, 10��node->variable->init_value�ɓ����Ă���
    Node *assign = static_cast<Node*>(calloc(1,sizeof(Node)));
    assign->kind = ND_ASSIGN;
    assign->lhs = node;
    assign->rhs = node->variable->init_value;
    return assign;
}

void read_type(DefineFuncOrVariable *def_first_half) {
    if (!def_first_half) {
        error("invalid def_first_half variable.");
    }

    Type *type = def_first_half->type;

    // �z�񂩃`�F�b�N
    while (consume("[")) {
        Type *t = static_cast<Type*>(calloc(1,sizeof(Type)));
        t->ty = ARRAY;
        t->ptr_to = type;
        Token *array_num = NULL;
        t->array_size = 0;
        if (array_num = consume_kind(TK_NUM)) {
            t->array_size = array_num->val;
        }
        type = t;
        expect("]");
    }
    def_first_half->type = type;
}

int get_size(Type *type) {
    if (type->ty == STRUCT) {
        return type->byte_size;
    }
    if (type->ty == ARRAY) {
        if (type->array_size == 0) {
            error("array size is not specified.");
        }
        return get_size(type->ptr_to) * type->array_size;
    }
    return type->ty == PTR ? 8 : type->ty == CHAR ? 1 : 4;
}

// �܂���`����Ă��Ȃ��ϐ��̒�`���s��
// int *foo; int *foo() {} �Ȃǂ��������ꍇ�Aint *foo�̕����܂ł�def_first_half
Node *define_variable(DefineFuncOrVariable *def_first_half, Variable **variable_list) {
    read_type(def_first_half);
    Type *type = def_first_half->type;

    // ��������
    Node *init_value = NULL;
    if (consume("=")) {
        if (consume("{")) {
            // �z��̏�����
            // int a[3] = {1,2,3} �̂悤�ȏꍇ
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
            if (type->array_size < i) { // arr[] = {1,2} �̂悤�ȏꍇ
                type->array_size = i + 1;
            }
            for (i = i + 1; i < type->array_size; i++) { // arr[5] = {1,2} �̂悤�ȏꍇ
                init_value->block[i] = new_node_num(0);
            }
        } else {
            // �萔���̏ꍇ
            // int a = 3; �̂悤�ȏꍇ
            init_value = expr();

            // string�̏ꍇ
            // char arr[] = "abc"; �̂悤�ȏꍇ
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
    node->byte_size = get_size(type);

    Variable *local_variable = find_varable(def_first_half->ident);
    if (local_variable != NULL) {
        error("redefined variable: %s", node->variable_name);
    }

    // FIXME
    if (locals == variable_list) {
        node->kind = ND_LOCAL_VARIABLE;
    } else {
        node->kind = ND_GLOBAL_VARIABLE_USE;
    }

    local_variable = static_cast<Variable*>(calloc(1,sizeof(Variable)));
    local_variable->next = variable_list[current_func];
    local_variable->name = def_first_half->ident->str;
    local_variable->len = def_first_half->ident->len;
    local_variable->init_value = init_value;
    if (variable_list[current_func] == NULL) {
        local_variable->offset = 8;
    } else {
        local_variable->offset = variable_list[current_func]->offset + node->byte_size;
    }
    local_variable->type = type;

    node->offset = local_variable->offset;
    node->type = local_variable->type;
    node->variable = local_variable;
    variable_list[current_func] = local_variable;
    char name[100] = {0};
    memcpy(name, def_first_half->ident->str, def_first_half->ident->len);
    // fprintf(stderr, "*NEW VARIABLE* %s\n", name);
    return node;
}

// ��`�ς݂̕ϐ����Q�Ƃ���
Node *variable(Token *tok) {
    Node *node = static_cast<Node*>(calloc(1,sizeof(Node)));
    node->variable_name = static_cast<char*>(calloc(100,sizeof(char)));
    memcpy(node->variable_name, tok->str, tok->len);

    Variable *local_variable = find_varable(tok);
    if (local_variable == NULL) {
        error("undefined variable: %s", node->variable_name);
    }

    if (local_variable->kind == Variable::LOCAL_VARIABLE) {
        node->kind = ND_LOCAL_VARIABLE;
    } else {
        node->kind = ND_GLOBAL_VARIABLE_USE;
    }
    node->offset = local_variable->offset;
    node->type = local_variable->type;

    while (true) {
        // a[3] �� *(a + 3) �Ɠ���  tok��a�܂Ŏ��Ă���
        if (consume("[")) {
            // node�͌���a 
            // add��lhs��a, rhs��3*4������(4��a��int�̏ꍇ�̃T�C�Y)
            // add�ɂ�(a + 3)������
            Node *add = static_cast<Node*>(calloc(1,sizeof(Node)));
            add->kind = ND_ADD;
            add->lhs = node;
            if (node->type && node->type->ty != INT) {
                int n = node->type->ptr_to->ty == INT ? 4 
                        : node->type->ptr_to->ty == CHAR ? 1
                        : 8;
                // �^�̃T�C�Y��expr()�̒l��������������rhs�ɓ����
                add->rhs = new_binary(ND_MUL, expr(), new_node_num(n));
            }

            // �V����node������āAlhs��(a + 3)��add������
            // �ŏI�I��node��Ԃ����߁Anode��V�����X�V���Ă���
            node = static_cast<Node*>(calloc(1,sizeof(Node)));
            node->kind = ND_DEREF;
            node->lhs = add;

            expect("]");
            continue;
        }

        if (consume(".")) {
            node = struct_reference(node);
            continue;
        }

        if (consume("->")) {
            // x->y �� (*x).y �Ɠ���
            Type *t = node->type->ptr_to;
            node = new_binary(ND_DEREF, node, NULL);
            node->type = t;
            node = struct_reference(node);
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
    char token_str[100];
    memcpy(token_str, tok->str, tok->len);
    for (Member *m = type->member_list; m; m = m->next) {
        if (memcmp(m->name, token_str, tok->len) == 0) {
            return m;
        }
    }
    error("undefined member");
    return NULL;
}

// �ϐ��𖼑O�Ō�������B
Variable *find_varable(Token *tok) {
    // �܂����[�J���̕ϐ�������
    for (Variable *var = locals[current_func]; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            var->kind = Variable::LOCAL_VARIABLE;
            return var;
        }
    }

    // ���[�J���ϐ��ɕϐ�����������Ύ��ɃO���[�o���ϐ�������
    for (Variable *var = globals[0]; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            var->kind = Variable::GLOBAL_VARIABLE;
            return var;
        }
    }

    // �ǂ�����Ȃ����NULL��Ԃ�
    return NULL;
}

// �A���C�����g�𑵂���
// struct {int a; char b1; char b2; int c;}
// a 4byte
// b1 1byte
// b2 1byte
//    2byte <- 4byte�ɂȂ�悤�ɃA���C�����g�𑵂���
// c 4byte
// align_to(12, 8) = 16
// byte_size���ł��߂�align�̔{���ɐ؂�グ��
int align_to(int byte_size, int align) {
    return (byte_size + align - 1) & ~(align - 1); // 2�ׂ̂���ŃA���C�����g�𑵂���
}

void push_struct_tag_to_global(const char* prefix, Token *tok, Type *type) {
    char *tag_name = static_cast<char*>(calloc(100,sizeof(char)));
    if (prefix) {
        memcpy(tag_name, prefix, strlen(prefix));
        memcpy(tag_name + strlen(prefix), " ", 1);
        memcpy(tag_name + strlen(prefix) + 1, tok->str, tok->len);
    } else {
        memcpy(tag_name, tok->str, tok->len);
    }
    StructTag *tag = static_cast<StructTag*>(calloc(1,sizeof(StructTag)));
    tag->name = tag_name;
    tag->type = type;
    if (struct_tags) {
        tag->next = struct_tags;
    }
    struct_tags = tag;
}

StructTag *find_tag(const char* prefix, Token *tok) {
    char tag_name[100] = {0};
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
    return NULL;
}

bool define_typedef() {
    if (!consume_kind(TK_TYPEDEF)) {
        return false;
    }
    DefineFuncOrVariable *def_first_half = read_define_first_half();
    expect(";");
    push_struct_tag_to_global(NULL, def_first_half->ident, def_first_half->type);
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

        if (ident) {
            push_struct_tag_to_global("enum", ident, int_type());
        }
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