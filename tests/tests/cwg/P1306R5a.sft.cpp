//type:fp
//options_all:--c++26 -tused -A
// P1306R5 -- Expansion statements.
//
// An expansion statement
//
//   template for (init-statement_opt for-range-declaration
//                 : expansion-initializer) compound-statement
//
// instantiates its compound-statement once per element of the
// expansion-initializer, each instantiation having its own copy of the
// for-range-declaration.

template <class T, class U> constexpr bool is_same = false;
template <class T> constexpr bool is_same<T, T> = true;

// Enumerating: the elements may have unrelated types.
constexpr int enumerating() {
  int result = 0;
  template for (auto g : {1, 2U, 3LL}) {
    result += (int)sizeof(g);
  }
  return result;
}
static_assert(enumerating() ==
              (int)(sizeof(int) + sizeof(unsigned) + sizeof(long long)));

// Iterating: the expansion-initializer must be a constant expression.
template <class T, int N> struct array {
  T data[N];
  constexpr const T *begin() const { return data; }
  constexpr const T *end() const { return data + N; }
};
constexpr array<int, 3> arr{{1, 2, 3}};
constexpr int iterating() {
  int result = 0;
  template for (constexpr int s : arr) {
    result += sizeof(char[s]);
  }
  return result;
}
static_assert(iterating() == 6);

// Destructuring: a class, a tuple-like type, or an array.
struct S { int i; short s; };
constexpr int destructuring() {
  int result = 0;
  S s{1, 2};
  template for (auto x : s) {
    result += (int)sizeof(x);
  }
  int a[] = {1, 2, 3, 4};
  template for (auto x : a) {
    result += x;
  }
  return result;
}
static_assert(destructuring() == (int)(sizeof(int) + sizeof(short)) + 10);

// A "break" terminates the whole expansion statement and a "continue"
// proceeds to the next expansion.
constexpr int break_and_continue(int x) {
  int result = 0, n = 3;
  while (n > 0) {
    --n;
    ++result;
    template for (constexpr int value : {10, 20, 30}) {
      result += value;
      if (x == 0) break;
      else if (x == 1) continue;
      result += 42;
    }
  }
  return result;
}
static_assert(break_and_continue(0) == 3 * (1 + 10));
static_assert(break_and_continue(1) == 3 * (1 + 10 + 20 + 30));
static_assert(break_and_continue(2) == 3 * (1 + 10 + 20 + 30 + 3 * 42));

// An init-statement is scanned once; the declaration it introduces is in
// scope in each expansion.
constexpr int init_statement() {
  int result = 0;
  template for (int k = 100; auto x : {1, 2, 3}) {
    result += x + k;
  }
  return result;
}
static_assert(init_statement() == 306);

// The declaration may be "constexpr", a reference, or a structured binding
// declaration.
constexpr int declaration_forms() {
  int a = 1, b = 2;
  template for (auto &e : {a, b}) { e += 10; }
  struct P { int x; long y; };
  P ps[2] = {{1, 2}, {3, 4}};
  int result = a + b;
  template for (auto [x, y] : ps) { result += x + (int)y; }
  template for (constexpr int v : {5, 6}) {
    static_assert(v == 5 || v == 6);
    result += v;
  }
  return result;
}
static_assert(declaration_forms() == 23 + 10 + 11);

// No expansion is performed when the expansion-initializer is empty.
struct Empty { };
constexpr int no_expansions() {
  int result = 0;
  template for (auto x : {}) { result = 1; }
  template for (auto x : Empty{}) { result = 2; }
  return result;
}
static_assert(no_expansions() == 0);

// Expansion statements nest, and the body of one is scanned once per
// expansion of the enclosing one.
constexpr int nested() {
  int result = 0;
  template for (auto i : {1, 2, 3}) {
    template for (auto j : {10, 20}) { result += i * j; }
  }
  return result;
}
static_assert(nested() == 180);

// In a template, the expansion is performed when the template is
// instantiated.
template <class... Ts> constexpr int sizes() {
  int result = 0;
  template for (auto t : {Ts{}...}) { result += (int)sizeof(t); }
  return result;
}
static_assert(sizes<char, int, double>() ==
              (int)(sizeof(char) + sizeof(int) + sizeof(double)));

template <class T> constexpr int members(T v) {
  int result = 0;
  template for (auto m : v) { result += (int)m; }
  return result;
}
static_assert(members(S{3, 4}) == 7);

// The type of the variable is deduced in each expansion, so the body is
// checked against each element in turn.
constexpr bool types() {
  bool ok = true;
  template for (auto g : {1, 2L}) {
    ok = ok && (is_same<decltype(g), int> || is_same<decltype(g), long>);
  }
  return ok;
}
static_assert(types());

//paper: P1306R5
//title: Expansion statements
//meeting: Sofia 6/25
