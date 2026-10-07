//type:rp
//use_system_includes: true
//edg_header_pack: exp_meta
//options:--set_flag=definition_tokens:--set_flag=definition_tokens --parse_templates --no_defer_parse_function_templates:--c++26
//options_all:--set_flag=injection

/*
std::meta::resolved_operator_of gives, for an operator token of a definition
(from std::meta::definition_tokens_of), the operator function that overload
resolution selected for it: a member function, a function found by
argument-dependent lookup, or (in C++20) a rewritten comparison, possibly with
reversed operands.  For a built-in operator, the result is the token itself.
is_resolved_through_adl, is_rewritten_operator, and has_reversed_operands
describe how the function was found.

Test 1 records the definitions with the "definition_tokens" flag; test 3 uses
only the [[edg::retain_tokens]] attribute.  Test 2 also covers a function
template, whose definition must be parsed where it appears for its operators
to be resolved.
*/

#include <experimental/meta>
#include <compare>
#include <cstdio>
#include <string>
#include <string_view>

using namespace std::meta;

struct S {
  int v;
  S operator+(S o) const { return {v + o.v}; }
};
namespace ns {
  struct N { int v; };
  N operator+(N a, N b) { return {a.v + b.v}; }
  bool operator==(N a, N b) { return a.v == b.v; }
  std::strong_ordering operator<=>(N a, int b) { return a.v <=> b; }
}
struct V {
  int a[4];
  int &operator[](int i) { return a[i]; }
};
struct P {
  S s;
  S *operator->() { return &s; }
};

[[edg::retain_tokens]]
int f(int a, int b, S s1, S s2, ns::N n1, ns::N n2, V v, P p) {
  int x = a + b;
  S s3 = s1 + s2;
  ns::N n3 = n1 + n2;
  bool ne = n1 != n2;
  bool lt = 1 < n1;
  v[1] = x;
  return p->v + s3.v + n3.v + ne + lt;
}

consteval std::string number(unsigned long n) {
  std::string s;
  do { s.insert(s.begin(), char('0' + n % 10)); n /= 10; } while (n != 0);
  return s;
}

// A token that, preceded by "operator", forms an operator-function-id.
consteval bool is_operator_token(info t) {
  return token_kind_of(^^{ operator \{t} }) ==
         token_kind::operator_function_id;
}

// One line per operator token of the body: the selected function (or
// "built-in") and how it was found.
consteval std::string describe(info body) {
  std::string out;
  for (info t : tokens_of(body)) {
    if (!is_operator_token(t) && token_spelling_of(t) != "[") continue;
    info r = resolved_operator_of(t);
    out += "line ";
    out += number(token_location_of(t).line());
    out += ": '";
    out += token_spelling_of(t);
    out += "' -> ";
    if (r == t) {
      out += "built-in";
    } else {
      out += display_string_of(r);
      out += " (operator_of: ";
      out += symbol_of(operator_of(r));
      out += ")";
      if (is_resolved_through_adl(t)) out += " [ADL]";
      if (is_rewritten_operator(t)) out += " [rewritten]";
      if (has_reversed_operands(t)) out += " [reversed]";
    }
    out += "\n";
  }
  return out;
}

// The n-th token of the body of fn spelled sp.
consteval info nth_token(info fn, std::string_view sp, int n) {
  for (info t : tokens_of(definition_tokens_of(fn)))
    if (token_spelling_of(t) == sp && n-- == 0) return t;
  return ^^{};
}

static_assert(parent_of(resolved_operator_of(nth_token(^^f, "+", 1))) == ^^S);
static_assert(parent_of(resolved_operator_of(nth_token(^^f, "+", 2))) ==
              ^^ns);
static_assert(is_resolved_through_adl(nth_token(^^f, "+", 2)));
static_assert(!is_resolved_through_adl(nth_token(^^f, "+", 1)));
static_assert(operator_of(resolved_operator_of(nth_token(^^f, "!=", 0))) ==
              op_equals_equals);
static_assert(has_reversed_operands(nth_token(^^f, "<", 0)));
// For a built-in operator, the result is the token itself.
consteval bool is_built_in(info fn, std::string_view sp, int n) {
  info t = nth_token(fn, sp, n);
  return resolved_operator_of(t) == t && !is_resolved_through_adl(t);
}
static_assert(is_built_in(^^f, "+", 0));
static_assert(!is_built_in(^^f, "+", 1));

#if TEST_NUMBER == 2
// In a template, an operator that does not depend on a template parameter is
// resolved at the definition.
template <class T> int g(T t, S s1, S s2) {
  S s3 = s1 + s2;
  return t + s3.v;
}
static_assert(parent_of(resolved_operator_of(nth_token(^^g, "+", 0))) ==
              ^^S);
#endif

int main() {
  std::fputs(define_static_string(describe(definition_tokens_of(^^f))),
             stdout);
#if TEST_NUMBER == 2
  std::printf("g: s1 + s2 -> %s\n",
              define_static_string(display_string_of(
                resolved_operator_of(nth_token(^^g, "+", 0)))));
#endif
}
