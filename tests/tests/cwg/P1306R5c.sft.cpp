//type:rp
//options_all:--c++26 --set_flag=reflection
//use_system_includes: true
//edg_header_pack: exp_meta
// P1306R5 -- Expansion statements combined with reflection (P2996).
//
// The canonical use of an iterating expansion statement: iterate over the
// reflections of the enumerators of an enumeration and of the non-static
// data members of a class.  The std::vector returned by the std::meta query
// is turned into a static array (define_static_array) held by a
// "static constexpr" variable, so that the range is usable in a constant
// expression.
//
// Expected output:
//   Red = 1
//   Green = 2
//   Blue = 4
//   sum = 7
//   x = 10
//   y = 20
//   name = pt
//   fields = 3

#include <experimental/meta>
#include <cstdio>
#include <string_view>

using namespace std::meta;

enum class Color { Red = 1, Green = 2, Blue = 4 };

struct Point {
  int x;
  int y;
  const char *name;
};

// Iterate over the enumerators of an enumeration: the expansion variable is
// constexpr, so the reflection can be spliced in the body.
template <class E>
int print_enumerators() {
  int sum = 0;
  static constexpr auto enumerators =
                                   define_static_array(enumerators_of(^^E));
  template for (constexpr auto e : enumerators) {
    std::printf("%s = %d\n", identifier_of(e).data(), (int)[:e:]);
    sum += (int)[:e:];
  }
  return sum;
}

// Iterate over the non-static data members of a class: the member reflection
// is spliced to access the field of a given object.
template <class T>
void print_fields(const T &obj) {
  static constexpr auto members = define_static_array(
                    nonstatic_data_members_of(^^T, access_context::unchecked()));
  template for (constexpr auto m : members) {
    if constexpr (type_of(m) == ^^int) {
      std::printf("%s = %d\n", identifier_of(m).data(), obj.[:m:]);
    } else {
      std::printf("%s = %s\n", identifier_of(m).data(), obj.[:m:]);
    }
  }
}

// The number of members can be computed at compile time by counting the
// expansions.
template <class T>
consteval int count_fields() {
  int n = 0;
  static constexpr auto members = define_static_array(
                    nonstatic_data_members_of(^^T, access_context::unchecked()));
  template for (constexpr auto m : members) {
    ++n;
  }
  return n;
}
static_assert(count_fields<Point>() == 3);

int main() {
  int sum = print_enumerators<Color>();
  std::printf("sum = %d\n", sum);
  Point pt{10, 20, "pt"};
  print_fields(pt);
  std::printf("fields = %d\n", count_fields<Point>());
  return sum == 7 ? 0 : 1;
}

//paper: P1306R5
//title: Expansion statements
//meeting: Sofia 6/25
