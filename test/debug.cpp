enum Piyo {
  PIYO_A = 10,
  PIYO_B,
  PIYO_C
};

int main() {
    enum Piyo2 {
    AAA = 10,
    BBB,
    CCC
  } aa;
  enum Piyo2 piyo2;
  piyo2 = AAA;
  return piyo2;
}