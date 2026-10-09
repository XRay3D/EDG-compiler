//type:rp
//use_system_includes: true
//edg_header_pack: exp_meta
//options_all:--set_flag=injection

/*
Iterate over the tokens of a token sequence (std::meta::tokens_of), logging
the kind, spelling, value, and line of each token, and reassemble the tokens
into an equivalent token sequence without a language extension: a token
sequence is concatenated by interpolating it into a new one,
"^^{ \{out} \{tok} }" (std::meta::list_builder does the same).
*/

#include <experimental/meta>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace std::meta;

consteval const char *kind_name(token_kind k) {
  switch (k) {
    case token_kind::identifier: return "identifier";
    case token_kind::keyword: return "keyword";
    case token_kind::punctuator: return "punctuator";
    case token_kind::integer_literal: return "integer";
    case token_kind::floating_literal: return "floating";
    case token_kind::character_literal: return "character";
    case token_kind::string_literal: return "string";
    case token_kind::boolean_literal: return "boolean";
    case token_kind::user_defined_literal: return "udl";
    case token_kind::interpolated: return "interpolated";
    case token_kind::operator_function_id: return "operator-function-id";
  }
  return "?";
}

consteval std::string number(unsigned long n) {
  std::string s;
  do { s.insert(s.begin(), char('0' + n % 10)); n /= 10; } while (n != 0);
  return s;
}

consteval bool is_literal(token_kind k) {
  return k != token_kind::identifier && k != token_kind::keyword &&
         k != token_kind::punctuator && k != token_kind::interpolated;
}

// One line per token: "kind 'spelling' line N [value=V]".
consteval std::string describe(info seq) {
  std::string out;
  for (info t : tokens_of(seq)) {
    token_kind k = token_kind_of(t);
    out += kind_name(k);
    out += " '";
    out += token_spelling_of(t);
    out += "' line ";
    out += number(token_location_of(t).line());
    if (is_literal(k)) {
      out += " value=";
      out += display_string_of(token_value_of(t));
    }
    out += "\n";
  }
  return out;
}

constexpr info seq = ^^{ int x = 0x10 + 'a';
  if (x > 3) f("s", 2.5, true); };

// Reassemble the tokens one at a time by interpolation.
consteval info rebuild(info s) {
  info out = ^^{};
  for (info t : tokens_of(s)) out = ^^{ \{out} \{t} };
  return out;
}

// The same with std::meta::list_builder (which interpolates as well).
consteval info rebuild_with_list_builder(info s) {
  list_builder out(^^{});
  for (info t : tokens_of(s)) out += t;
  return out;
}

// Rebuild, replacing each identifier "x" by "y".
consteval info rename_x(info s) {
  info out = ^^{};
  for (info t : tokens_of(s)) {
    if (token_kind_of(t) == token_kind::identifier &&
        token_spelling_of(t) == "x")
      out = ^^{ \{out} y };
    else
      out = ^^{ \{out} \{t} };
  }
  return out;
}

static_assert(token_spelling_of(rebuild(seq)) == token_spelling_of(seq));
static_assert(tokens_of(rebuild(seq)).size() == tokens_of(seq).size());
static_assert(token_spelling_of(rebuild_with_list_builder(seq)) ==
              token_spelling_of(seq));
static_assert(token_spelling_of(rename_x(^^{ x + x * z })) == "y + y * z");
static_assert(tokens_of(^^{}).empty());

// The standard queries display_string_of and source_location_of also work on
// token sequences: they give the text and the position of the tokens.
consteval bool standard_queries_agree(info s) {
  for (info t : tokens_of(s)) {
    if (display_string_of(t) != token_spelling_of(t)) return false;
    if (source_location_of(t).line() != token_location_of(t).line())
      return false;
    if (source_location_of(t).column() != token_location_of(t).column())
      return false;
  }
  return display_string_of(s) == token_spelling_of(s);
}
static_assert(standard_queries_agree(seq));

// An operator-function-id is one element of tokens_of (it is one name), and
// operator_of gives its std::meta::operators enumerator, as it does for the
// operator function that the name designates.
constexpr info ops = ^^{ a.operator+(b); v.operator[](1);
  operator new[](8); c += d; operator bitor; operator int; };

consteval std::string describe_operators(info s) {
  std::string out;
  for (info t : tokens_of(s)) {
    out += kind_name(token_kind_of(t));
    out += " '";
    out += token_spelling_of(t);
    out += "'";
    if (token_kind_of(t) == token_kind::operator_function_id) {
      out += " operator_of=";
      out += symbol_of(operator_of(t));
    }
    out += "\n";
  }
  return out;
}

static_assert(token_spelling_of(rebuild(ops)) == token_spelling_of(ops));
static_assert(operator_of(^^{ operator + }) == op_plus);
static_assert(operator_of(^^{ operator ( ) }) == op_parentheses);
static_assert(operator_of(^^{ operator delete [ ] }) == op_array_delete);
static_assert(operator_of(^^{ operator co_await }) == op_co_await);
static_assert(tokens_of(^^{ operator new [ ] x }).size() == 2);
static_assert(token_kind_of(^^{ operator int }) == token_kind::keyword);

// A bare operator token is not a name, but "operator" can be put in front of
// it to classify it.
consteval bool is_operator_token(info t) {
  return token_kind_of(^^{ operator \{t} }) ==
         token_kind::operator_function_id;
}
consteval operators operator_of_token(info t) {
  return operator_of(^^{ operator \{t} });
}
static_assert(is_operator_token(tokens_of(^^{ c += d })[1]));
static_assert(operator_of_token(tokens_of(^^{ c += d })[1]) == op_plus_equals);
static_assert(!is_operator_token(tokens_of(^^{ c += d })[0]));
static_assert(!is_operator_token(tokens_of(^^{ a :: b })[1]));

// token_value_of gives a reflection of a value, which can be spliced (a token
// itself cannot be).  "\val(...)" yields an interpolated token, as does the
// operand of "\[:...:]"; its value is the reflection interpolated, or the
// reflected constant for a value of another type.
constexpr int limit = 42;
static_assert([:token_value_of(tokens_of(^^{ 0x10 })[0]):] == 16);
static_assert(token_value_of(tokens_of(^^{ 'a' })[0]) ==
              reflect_constant('a'));
constexpr info val_limit = tokens_of(^^{ \val(^^limit) })[0];
static_assert(token_kind_of(val_limit) == token_kind::interpolated);
static_assert(token_value_of(val_limit) == ^^limit);
static_assert([:token_value_of(val_limit):] == 42);
constexpr info val_42 = tokens_of(^^{ \val(42) })[0];
static_assert(token_kind_of(val_42) == token_kind::interpolated);
static_assert(token_value_of(val_42) == reflect_constant(42));
static_assert(token_kind_of(tokens_of(^^{ \[:^^limit:] })[1]) ==
              token_kind::interpolated);

// An identifier has no value; the entity it names is found by injecting it.
consteval {
  queue_injection(^^{ constexpr info found_limit =
                        ^^\{tokens_of(^^{ limit })[0]}; });
}
static_assert(found_limit == ^^limit);

// A function injected from reassembled (and edited) tokens.
consteval {
  queue_injection(rename_x(^^{ int answer() { int x = 6; return x * 7; } }));
}

int main() {
  std::fputs(define_static_string(describe(seq)), stdout);
  constexpr std::size_t count = tokens_of(seq).size();
  std::printf("count=%zu\n", count);
  std::printf("rebuilt: %s\n", define_static_string(token_spelling_of(
                                 rename_x(seq))));
  std::printf("answer()=%d\n", answer());
  std::fputs(define_static_string(describe_operators(ops)), stdout);
}
