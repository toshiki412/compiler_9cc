// テストに使うグローバル変数の定義
// 頭に定義しないと、現在のコンパイラはcurrent_funcが関数が出てくるたびに1ずつ増えるためエラーになる
// 現状はcurrent_funcが0のときグローバル変数とするという実装になっている
int global_a;
int global_b[10];
int global_var_init = 3;
int global_array_init[5] = {0, 1, 2, 3, 4};
char global_char_init[5] = {5, 6, 7, 8, 11};
char *global_message = "foo";
char global_message_array[4] = "bar";

struct Hoge {
  int a;
  char b;
  int c;
};

typedef int MyInt;
typedef char* MyString;
typedef struct Hoge MyHoge;

enum Piyo {
  PIYO_A = 10,
  PIYO_B,
  PIYO_C
};

int assert(int expected, int actual) {
    if (expected == actual) {
        printf("|"); // printfのエラーは#includeがまだないため修正不要
        return 0;
    }
    printf("%d expected, but got %d\n", expected, actual);
    exit(1);
}

int fail() {
  printf("called fail!\n");
  exit(1);
}

int ok() {
  printf("|");
}

// 四則演算のテスト
int test_calc() {
  assert(42, 42);
  assert(21, 5+20-4);
  assert(41, 12 + 34 - 5 );
  assert(47, 5+6*7);
  assert(15, 5*(9-6));
  assert(4, (3+5)/2);
  assert(10, -10+20);
  assert(10, - -10);
  assert(10, - - +10);
}

// 比較演算のテスト
int test_compare() {
  assert(0, 0==1);
  assert(1, 42==42);
  assert(1, 0!=1);
  assert(0, 42!=42);

  assert(1, 0<1);
  assert(0, 1<1);
  assert(0, 2<1);
  assert(1, 0<=1);
  assert(1, 1<=1);
  assert(0, 2<=1);

  assert(1, 1>0);
  assert(0, 1>1);
  assert(0, 1>2);
  assert(1, 1>=0);
  assert(1, 1>=1);
  assert(0, 1>=2);
}

// 変数のテスト
int test_variable() {
  int foo;
  int bar;
  foo = 3;
  bar = 5 * 6 - 8;
  assert(3, foo);
  assert(bar, 22);
  assert(14, foo + bar / 2);
}

// returnのテストのための内部関数
int inner_test_return() {
  int a;
  int b;
  a = 3;
  b = 5 * 6 - 8;
  return a + b / 2;
}

// multi returnのテストのための内部関数
int inner_multi_return() {
  return 5;
  return 8;
}

// returnのテスト
int test_return() {
  assert(14, inner_test_return());
  assert(5, inner_multi_return());
}

// ifのテスト
int test_if() {
  if (3 != 3) {
    fail();
  }

  int a;
  a = 3;
  if (a == 3) {
    ok();
    return 0;
  }
  fail();
}

// whileのテスト
int test_while() {
  int i;
  i = 0;
  while (i <= 10) i = i + 1;
  assert(11, i);
}

// forのテスト
int test_for() {
  int a;
  int i;
  a = 0;
  for (i = 0; i < 10; i = i + 1) a = a + 2;
  assert(10, i);
  assert(20, a);

  int b;
  b = 0;
  for (;b < 10;) b = b + 1;
  assert(10, b);
}

// 複数の制御構文のテストのための内部関数
int inner_multi_c() {
   int b;
  b = 0;
  for(;;b = b + 1) if (b == 5) return b;
}

// 複数の制御構文のテスト
int test_multi_control_stmt() {
  int a;
  a = 3;
  if (a == 1) fail();
  if (a == 2) fail();
  if (a == 3) ok();
  assert(3, a);

  assert(5, inner_multi_c());
}

// ブロックのテストのための内部関数
int inner_block() {
  int a;
  a = 0;
  for(;;) {
    a = a + 1;
    if (a == 5) return 10;
  }
  return 2;
}

// ブロックのテスト
int test_block() {
  assert(10, inner_block());
}

// 関数呼び出しのテストのための関数
int foo() {
  return 1;
}

int bar(int x, int y) {
  return x + y;
}

int piyo(int x, int y, int z) {
  return x + y + z;
}

// 関数呼び出しのテスト
int test_func() {
  assert(1, foo());
  assert(7, bar(3, 4));
  assert(12, piyo(3, 4, 5));
}

// ポインタのテストのための内部関数
int inner_test_pointer01() {
  int x;
  int *y;
  x = 3;
  y = &x;
  assert(3, *y);
}

int inner_test_pointer02() {
  int x;
  int y;
  int *z;
  x = 3;
  y = 5;
  z = &y + 4;
  assert(3, *z);
}

// ポインタのテスト
int test_pointer() {
  inner_test_pointer01();
  inner_test_pointer02();
}

// 関数定義のテストのための内部関数
int inner_test_func_def1(int a, int b) { return a + b; }
int inner_test_func_def2(int a, int b, int c) { return a + c; }

// 関数定義のテスト
int test_func_def() {
  assert(3, inner_test_func_def1(1, 2));
  assert(4, inner_test_func_def2(1, 2, 3));
}

// 再帰関数のテストのための内部関数
int sum(int n) {
  if (n < 0) return 0;
  return n + sum(n - 1);
}

// 再帰関数のテスト
int test_func_def_recursive() {
  int a;
  a = 10;
  assert(55, sum(a));
}

// ポインタ演算のテスト
int test_pointer_calc() {
  int *p;
  alloc4(&p, 1, 2, 4, 8);

  int *q;
  q = p + 2;
  assert(4, *q);

  q = p + 3;
  assert(8, *q);

  q = q - 2;
  assert(2, *q);
}

// sizeofのテスト
int test_sizeof() {
  int x;
  assert(4, sizeof(x));

  int *y;
  assert(8, sizeof(y));

  assert(8, sizeof(y + 3));

  assert(4, sizeof(*y));

  assert(4, sizeof(1));
}

// 配列のテスト
int test_array() {
  int a[2];
  *a = 1;
  *(a + 1) = 2;
  int *p;
  p = a;
  assert(3, *p + *(p + 1));
}

// 配列アクセスのテスト
int test_array_access() {
  int a[2];
  a[0] = 1;
  a[1] = 2;
  int *p;
  p = a;
  assert(3, p[0] + p[1]);
}

// グローバル変数のテスト
int test_global_variable() {
  global_a = 10;
  assert(10, global_a);
}

// charのテスト
int test_char() {
  char x[3];
  x[0] = -1;
  x[1] = 2;
  int y;
  y = 4;
  assert(3, x[0] + y);
  assert(1, sizeof(x[0]));
}

// 文字列のテスト
int test_string() {
  char *a;
  a = "abcd";
  assert(98, a[1]); // 98はasciiの'b'
}

// グローバル変数の初期化式のテスト
int test_gvar_init() {
  assert(3, global_var_init);

  assert(0, global_array_init[0]);
  assert(4, global_array_init[4]);

  assert(5, global_char_init[0]);
  assert(11, global_char_init[4]);

  assert(102, global_message[0]); //'f'のasciiは102
  assert(111, global_message[1]); //'o'のasciiは111
  
  assert(98, global_message_array[0]); //'b'のasciiは98
  assert(114, global_message_array[2]); //'r'のasciiは97
}

// ローカル変数の初期化式のテスト
int test_lvar_init() {
  int a = 10;
  int b[3] = {1, 2, bar(9, 4)};
  int c[] = {2, 3};
  int d[5] = {8};
  char abc[10] = "abc";
  char efg[] = "efg";

  assert(10, a);

  assert(1, b[0]);
  assert(2, b[1]);
  assert(13, b[2]);

  assert(2, c[0]);
  assert(3, c[1]);

  assert(8, d[0]);
  assert(0, d[1]);
  assert(0, d[4]);

  assert(97, abc[0]); //'a'のasciiは97
  assert(98, abc[1]); //'b'のasciiは98
  assert(99, abc[2]); //'c'のasciiは99
  assert(0, abc[3]); //文字列の終端は0
  assert(0, abc[9]);

  assert(101, efg[0]); //'e'のasciiは101
  assert(102, efg[1]); //'f'のasciiは102
  assert(103, efg[2]); //'g'のasciiは103
  assert(0, efg[3]); //文字列の終端は0
}

// 構造体のテスト
int test_struct() {
  struct {
    int a;
    int b;
  } abc;

  abc.a = 10;
  abc.b = 20;
  assert(10, abc.a);
  assert(20, abc.b);
}

int test_sturct_alignment() {
  struct StTest {
    int e;
    char f;
    int g;
  } efg;

  int size = &efg.g - &efg.e;
  assert(8, size);

  struct StTest efg2;
  int size2 = &efg2.g - &efg2.e;
  assert(8, size2);

}

int test_struct_arrow() {  
  struct Hoge {
    int a;
    char b;
    int c;
  } hoge;
  hoge.a = 10;

  struct Hoge *p = &hoge;
  assert(10, p->a);
}

int test_typedef() {
  MyInt a;
  a = 10;
  // MyString b = "abc"; //これがあるとエラー
  assert(10, a);

  MyHoge hoge;
  hoge.a = 10;
  assert(10, hoge.a);
}

int test_enum() {
  enum Piyo2 {
    AAA = 10,
    BBB,
    CCC
  } aa;
  enum Piyo2 piyo2;
  piyo2 = AAA;
  assert(10, piyo2);
  assert(11, BBB);
  assert(12, CCC);
}

int test_break() {
  int i;
  i = 0;
  while (1) {
    i = i + 1;
    if (i == 5) {
      break;
    }
  }
  assert(5, i);

  i = 0;
  for (;;) {
    i = i + 1;
    if (i == 3) {
      break;
    }
  }
  assert(3, i);

  i = 0;
  int j = 0;
  for (;;) {
    i = i + 1;
    if (i == 3) {
      break;
    }
    for (;;) {
      if (j == 4) {
        break;
      }
      j = j + 1;
    }
  }
  assert(3, i);
  assert(4, j);
}

int test_continue() {
  int i = 0;
  int j = 0;
  while(i < 10) {
    i = i + 1;
    if (i > 5) {
      continue;
    }
    j = j + 1;
  }

  assert(5, j);
  assert(10, i);

  i = 0;
  j = 0;
  for (; i<10; i = i + 1) {
    if (i > 5) {
      continue;
    }
    j = j + 1;
  }

  assert(10, i);
  assert(5, j);
}

int test_addeq() {
  int a = 10;
  a += 5;
  assert(15, a);
}

int test_subeq() {
  int a = 10;
  a -= 5;
  assert(5, a);
}

int test_muleq() {
  int a = 10;
  a *= 5;
  assert(50, a);
}

int test_diveq() {
  int a = 10;
  a /= 5;
  assert(2, a);
}

int test_addeq_ptr() {
  int a[2] = {2,4}; // 配列の大きさが5以上だとエラーになる
  int *p;
  p = &a;
  assert(2, *p);

  p += 1;
  assert(4, *p);

  p -= 1;
  assert(2, *p);
}

int test_plusplus() {
  int a = 10;
  a++;
  assert(11, a);
  ++a;
  assert(12, a);

  assert(12, a++);
  assert(13, a);
  assert(14, ++a);
}

int test_minusminus() {
  int a = 10;
  a--;
  assert(9, a);
  --a;
  assert(8, a);

  assert(8, a--);
  assert(7, a);
  assert(6, --a);
}

int main() {

  test_calc();
  test_compare();
  test_variable();
  test_return();
  test_if();
  test_while();
  test_for();
  test_multi_control_stmt();
  test_block();
  test_func();
  test_pointer();
  test_func_def();
  test_func_def_recursive();
  test_pointer_calc();
  test_sizeof();
  test_array();
  test_array_access();
  test_global_variable();
  test_char();
  test_string();
  test_gvar_init();
  test_lvar_init();
  test_struct();
  test_sturct_alignment();
  test_struct_arrow();
  test_typedef();
  test_enum();
  test_break();
  test_continue();
  test_addeq();
  test_subeq();
  test_muleq();
  test_diveq();
  test_addeq_ptr();
  test_plusplus();
  test_minusminus();

  printf("OK\n");
  return 0;
}