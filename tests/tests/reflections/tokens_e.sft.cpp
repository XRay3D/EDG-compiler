//type:fn
//use_system_includes: true
//edg_header_pack: exp_meta
//options_all:--set_flag=injection

/*
Diagnostics of the token query intrinsics: the argument is not a token
sequence, the token sequence is empty, the token has no value, and the tokens
of a definition were not recorded (the "definition_tokens" flag is not set and
the entity is not declared [[edg::retain_tokens]]).
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

// OK: the tokens of templates are always available.
template <class T> T id(T v) { return v; }
constexpr info ok = definition_tokens_of(^^id);
static_assert(token_spelling_of(ok) == "{ return v; }");
