//type:rp
//use_system_includes: true
//edg_header_pack: exp_meta
//options:--set_flag=definition_tokens:--c++26
//options_all:--set_flag=injection

/*
std::meta::definition_tokens_of returns the tokens of the definition of a
function (its body), a class (its member-specification, including the bodies
of member functions), or a namespace (the declarations of all its
definitions).  The tokens of templates are always available; those of other
entities are recorded for every definition with the "definition_tokens" flag
(test 1), and otherwise only for entities declared [[edg::retain_tokens]]
(test 2).
*/

#include <experimental/meta>
#include <cstdio>

using namespace std::meta;

int square(int x) { return x * x; }
struct Point {
  int x, y;
  int sum() const { return x + y; }
  struct Inner { int z; };
};
namespace geo { int one() { return 1; } }
namespace geo { int two() { return 2; } }
template <class T> T twice(T v) { return v + v; }
template <class T> struct Box { T value; T get() const { return value; } };
struct Holder {
  // A consteval block (rewritten internally) and a range-based for in the
  // recorded class body.
  consteval { for (int i : {1, 2}) (void)i; }
  int h() { return 5; }
};

[[edg::retain_tokens]] int marked(int a) { if (a) return 1; else return 0; }
struct [[edg::retain_tokens]] S { int v; int get() const { return v; } };
namespace [[edg::retain_tokens]] N { int f() { return 7; } }

#define SHOW(r) std::printf("%-16s: %s\n", #r, \
    define_static_string(token_spelling_of(definition_tokens_of(r))))

int main() {
#if TEST_NUMBER == 1
  SHOW(^^square);
  SHOW(^^Point);
  SHOW(^^Point::sum);
  SHOW(^^Point::Inner);
  SHOW(^^geo);
  SHOW(^^geo::two);
  SHOW(^^Holder);
  SHOW(^^Holder::h);
#endif
  SHOW(^^twice);
  SHOW(^^Box);
  SHOW(^^Box<int>);
  SHOW(^^Box<int>::get);
  SHOW(^^marked);
  SHOW(^^S);
  SHOW(^^S::get);
  SHOW(^^N);
  SHOW(^^N::f);
}
