//type:fn
//use_system_includes: true
//edg_header_pack: exp_meta
//options_all:--set_flag=injection --parse_templates --no_defer_parse_function_templates

/*
Diagnostics of the token query intrinsics: the argument is not a token
sequence, the token sequence is empty, the token has no value, the tokens of a
definition were not recorded (the "definition_tokens" flag is not set and the
entity is not declared [[edg::retain_tokens]]), operator_of is applied to a
token sequence that is not exactly one operator-function-id, and
resolved_operator_of is applied to a token that does not come from a recorded
definition or to a template-dependent operator.
*/

#include <experimental/meta>

using namespace std::meta;

int not_recorded() { return 0; }
namespace plain { int f(); }

constexpr std::size_t n1 = tokens_of(^^int).size();         // error
constexpr token_kind k1 = token_kind_of(^^{});               // error
constexpr info v1 = token_value_of(^^{ x });                 // error
constexpr auto s1 = token_spelling_of(^^not_recorded);       // error
constexpr auto l1 = token_location_of(^^{});                 // error
constexpr info d1 = definition_tokens_of(^^not_recorded);    // error
constexpr info d2 = definition_tokens_of(^^plain);           // error
constexpr info d3 = definition_tokens_of(^^int);             // error
// operator_of accepts a token sequence that is just an operator-function-id.
constexpr operators o1 = operator_of(^^{ + });               // error
constexpr operators o2 = operator_of(^^{ operator + x });    // error
constexpr operators o3 = operator_of(^^{ operator int });    // error

// resolved_operator_of needs a token of a recorded definition.
struct S { S operator+(S) const; };
[[edg::retain_tokens]] S k(S a, S b) { return a + b; }
template <class T> [[edg::retain_tokens]] T tt(T a) { return a + a; }
consteval info plus_of(info fn) {
  for (info t : tokens_of(definition_tokens_of(fn)))
    if (token_spelling_of(t) == "+") return t;
  return ^^{};
}
consteval info rebuilt(info t) { return ^^{ \{t} }; }
constexpr info r0 = resolved_operator_of(plus_of(^^k));      // OK
constexpr info r1 = resolved_operator_of(^^{ + });           // error
constexpr info r2 = resolved_operator_of(rebuilt(plus_of(^^k)));  // error
constexpr info r3 = resolved_operator_of(plus_of(^^tt));     // error

// OK: the tokens of templates are always available.
template <class T> T id(T v) { return v; }
constexpr info ok = definition_tokens_of(^^id);
static_assert(token_spelling_of(ok) == "{ return v; }");
