typedef struct NestType NestType;
struct NestType {
  struct NestType *next;
  int a;
};

int main() {
  NestType test_a;
  NestType *test_b;
  test_a.a = 10;
  test_b = &test_a;

  NestType test_c;
  test_c.a = 20;
  test_a.next = &test_c;

  NestType test_d;
  test_d.a = 30;
  test_c.next = &test_d;

  return 0;
}