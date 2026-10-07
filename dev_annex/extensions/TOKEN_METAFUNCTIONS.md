# Token Metafunctions (EDG Extension)

EDG implements P3294 token sequences (`^^{ ... }`, the interpolators
`\{...}`, `\(...)`, `\[...]`, `\str(...)`, and `queue_injection`).  The
metafunctions described here let a consteval function look *inside* a token
sequence: walk its tokens one by one, ask what each token is, and obtain the
tokens of a function, class, or namespace that is already defined.  Combined
with the existing interpolators, this is enough to rewrite code at compile time
and inject the result.  For example, it can produce a copy of a function with
logging added to every branch.

These are EDG extensions, not part of any WG21 proposal.

## Enabling

The metafunctions are declared in `<experimental/meta>`, namespace `std::meta`.
They need reflection and token injection:

```sh
eccp --c++26 --set_flag=reflection --set_flag=injection ...
```

The tokens of non-template definitions are recorded only on request.  Pass
`--set_flag=definition_tokens` or use the `[[edg::retain_tokens]]` attribute,
as described in [Recording definition tokens](#recording-definition-tokens).

All the metafunctions are `consteval`.  The vector returned by `tokens_of` is
allocated during constant evaluation.  So, as with `members_of`, use it inside
a consteval function or bind a derived value to a `constexpr` variable:

```cpp
constexpr std::size_t n = tokens_of(seq).size();   // OK
std::printf("%zu\n", tokens_of(seq).size());       // Error: not a constant
```

## Reference

```cpp
namespace std::meta {
  enum class token_kind {
    identifier, keyword, punctuator,
    integer_literal, floating_literal, character_literal,
    string_literal, boolean_literal, user_defined_literal,
    interpolated
  };

  consteval std::vector<info>    tokens_of(info tokens);
  consteval token_kind           token_kind_of(info token);
  consteval std::string_view     token_spelling_of(info tokens);
  consteval info                 token_value_of(info token);
  consteval std::source_location token_location_of(info token);
  consteval info                 definition_tokens_of(info r);
}
```

### `tokens_of`

```cpp
std::vector<info> tokens_of(info tokens);
```

`tokens` must reflect a token sequence.  The result holds one element per
token, in order.  Each element is itself a token sequence that contains just
that token.  Each token gets a fresh token sequence number, so the pieces can
be interpolated, repeated, reordered, and injected freely.  An empty sequence
yields an empty vector.

The one exception is an *operator-function-id*, which is a single name made of
several tokens.  It is returned as one element, as described in
[Operator names](#operator-names).

### `token_kind_of`

```cpp
token_kind token_kind_of(info token);
```

Returns the kind of the first element of `token`, in the sense of
`tokens_of`:

| `token_kind` | Tokens |
| --- | --- |
| `identifier` | Identifiers |
| `keyword` | Keywords (`int`, `if`, `return`, ...) |
| `punctuator` | Operators and punctuators (`{`, `::`, `+=`, ...) |
| `integer_literal` | Integer literals |
| `floating_literal` | Floating-point (and fixed-point) literals |
| `character_literal` | Character literals |
| `string_literal` | String literals |
| `boolean_literal` | `true` and `false` |
| `user_defined_literal` | User-defined literals |
| `interpolated` | A value spliced in with `\(...)` or a similar interpolator |
| `operator_function_id` | An *operator-function-id*, such as `operator +` |

### `token_spelling_of`

```cpp
std::string_view token_spelling_of(info tokens);
```

Returns the text of all the tokens of the sequence, separated by single spaces
where needed.  The string has static storage duration.  It can be compared,
passed to `define_static_string`, or used with `\str`.

The text of a literal is produced from its value, so it can differ from the
source.  For example, `2.5` is spelled `(2.5)`.  Tokens are those after macro
expansion.

### `token_value_of`

```cpp
info token_value_of(info token);
```

The first token of `token` must be a literal or an interpolated value.  The
result reflects its value as a constant, usable with `extract` or
`display_string_of`:

- For a literal, the result is the value of the literal.
- For a user-defined literal, the result is the value passed to the literal
  operator.
- For an interpolated reflection, the result is that reflection itself.

Identifiers, keywords, and punctuators have no value; calling it on them is an
error.

### `token_location_of`

```cpp
std::source_location token_location_of(info token);
```

Returns the source position of the first token: file, line, and column.  For
tokens obtained from `definition_tokens_of`, this is their position in the
original definition.  That makes it useful for log messages such as
`"line 12: while (x > 3)"`.

### Operator names

An *operator-function-id* is `operator` followed by an overloadable operator,
for example `operator +`, `operator [ ]`, `operator ( )`, `operator new [ ]`,
or `operator co_await`.  `tokens_of` returns it as one element of kind
`token_kind::operator_function_id`, because it is one name.  The front end
treats it the same way when it parses a declaration.

`operator_of` accepts such an element and returns the enumerator of
`std::meta::operators` whose *operator-function-id* it is.  This mirrors
P2996R13, where `operator_of` is defined through the *operator-function-id*
of an operator function:

```cpp
static_assert(operator_of(^^{ operator + }) == op_plus);
static_assert(operator_of(^^{ operator [ ] }) == op_square_brackets);
static_assert(operator_of(^^{ operator new [ ] }) == op_array_new);
static_assert(operator_of(^^{ operator bitor }) == op_pipe); // same token as |
static_assert(operator_of(^^S::operator+) == op_plus);        // unchanged
```

The sequence must be exactly one *operator-function-id*.  These are not, so
`operator_of` is not a constant for them:

- **A bare operator, such as `+`.**  It is a use of an operator, not the name
  of an operator function.
- **A conversion-function-id or literal-operator-id.**  `operator int` and
  `operator ""_km` have no enumerator in `operators`.  They stay separate
  tokens in `tokens_of`.
- **Extra tokens**, as in `operator + x`.

To classify a bare operator token `t`, put `operator` in front of it:

```cpp
consteval bool is_operator_token(info t) {
  return token_kind_of(^^{ operator \{t} }) ==
         token_kind::operator_function_id;
}
// operator_of(^^{ operator \{t} }) is op_plus_equals if t is "+=".
```

`is_operator_function` stays `false` for a token sequence, since a token is
not a function.

### Standard queries on token sequences

Two P2996 queries also accept a token sequence:

- `display_string_of(seq)` and `u8display_string_of(seq)` return the same text
  as `token_spelling_of(seq)`.
- `source_location_of(seq)` returns the same position as
  `token_location_of(seq)`.  An empty sequence has no position, so for it the
  call is not a constant expression.

P2996R13 leaves both results implementation-defined for reflections it does not
describe, so this does not change the behavior of any standard program.

### `definition_tokens_of`

```cpp
info definition_tokens_of(info r);
```

Returns a token sequence holding the tokens of the definition of the entity
reflected by `r`:

| Entity | Tokens returned |
| --- | --- |
| Function or member function | The body `{ ... }`, including a ctor-initializer and the handlers of a function-try-block |
| Class | The member-specification, without the enclosing braces, including member function bodies and nested classes |
| Namespace | The declarations of *all* its definitions, in order, without braces |
| Function template, class template | The template's own tokens |
| Specialization of a template, member of a class template specialization | The tokens of the template (template parameters are *not* substituted) |

The tokens are copied, so the result can be injected anywhere.  Each call
returns a fresh copy.

## Recording definition tokens

The front end always keeps the tokens of templates, because it needs them for
instantiation.  It discards the tokens of other definitions once they are
parsed.  `definition_tokens_of` therefore works for a non-template entity only
if its tokens were recorded.  Two modes are available, and they can be
combined:

- **`--set_flag=definition_tokens`** records the definition of every function,
  class, and namespace in the translation unit.  That includes the standard
  library headers, which costs memory and compile time.
- **`[[edg::retain_tokens]]`** records only the entity that carries the
  attribute.  No flag is needed:

  ```cpp
  [[edg::retain_tokens]] int f(int a) { if (a) return 1; return 0; }
  struct [[edg::retain_tokens]] S { int v; int get() const { return v; } };
  namespace [[edg::retain_tokens]] N { int g() { return 7; } }
  ```

  Definitions nested in a recorded one are recorded too.  For example,
  `definition_tokens_of(^^S::get)` and `definition_tokens_of(^^N::g)` both
  work.

Asking for tokens that were not recorded is an error:

```text
error: expression must have a constant value
note: the tokens of the definition of this entity are not available (they are
      recorded only with the "definition_tokens" flag or the
      [[edg::retain_tokens]] attribute)
```

## Reassembling tokens

No new operator is needed to concatenate token sequences.  Interpolating two
sequences into a new one already concatenates them:

```cpp
consteval info rebuild(info seq) {
  info out = ^^{};
  for (info t : tokens_of(seq)) out = ^^{ \{out} \{t} };
  return out;
}
static_assert(token_spelling_of(rebuild(seq)) == token_spelling_of(seq));
```

`std::meta::list_builder` works the same way, with a separator between pieces:
`list_builder out(^^{}); out += t;`.  Tokens can be dropped, replaced, or
inserted on the way:

```cpp
// Rename every identifier "x" to "y".
consteval info rename_x(info s) {
  info out = ^^{};
  for (info t : tokens_of(s))
    if (token_kind_of(t) == token_kind::identifier &&
        token_spelling_of(t) == "x")
      out = ^^{ \{out} y };
    else
      out = ^^{ \{out} \{t} };
  return out;
}

consteval {
  queue_injection(rename_x(^^{ int answer() { int x = 6; return x * 7; } }));
}
// answer() == 42
```

A token sequence literal cannot contain an unbalanced `{` or `}`.  When a
rewriter needs a lone brace, it can take one from the input.  For example,
`tokens_of(body).front()` is the `{` of a function body.

## `\str` with computed strings

The operand of `\str(...)` may now be a constexpr character array, such as the
result of `std::define_static_string`, in addition to a string literal.  That
lets a rewriter emit messages it computed:

```cpp
consteval info log_line(std::string_view msg) {
  std::string_view text = std::define_static_string(msg);
  return ^^{ trace(\str(text)); };
}
```

## Examples

The examples below are tests in `tests/tests/reflections/`.

### Logging what each token is (`tokens_a.sft.cpp`)

```cpp
constexpr info seq = ^^{ int x = 0x10 + 'a';
  if (x > 3) f("s", 2.5, true); };

consteval std::string describe(info seq) {
  std::string out;
  for (info t : tokens_of(seq)) {
    token_kind k = token_kind_of(t);
    out += kind_name(k);                       // A switch over token_kind.
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

int main() { std::fputs(define_static_string(describe(seq)), stdout); }
```

```text
keyword 'int' line 68
identifier 'x' line 68
punctuator '=' line 68
integer '0x10' line 68 value=16
punctuator '+' line 68
character ''a'' line 68 value='a'
...
string '"s"' line 69 value="s"
floating '(2.5)' line 69 value=(2.5)
boolean 'true' line 69 value=true
...
```

### Copying a function with logging (`tokens_c.sft.cpp`)

A small consteval token rewriter walks a function body statement by statement.
It does the following:

- It logs on entry to every block.
- It puts the branches of `if`/`else` and the bodies of `for`, `while`, and
  `do` in braces, so that branches without braces get room for a log line,
  and logs which branch or iteration runs.
- It logs before every `switch` and after every `case` and `default` label.

The rewritten body is then injected under a new name:

```cpp
int classify(int n) {
  int score = 0;
  for (int i = 0; i < n; ++i)
    if (i % 2 == 0) score += 2; else score -= 1;
  while (score > 3)
    score -= 3;
  switch (score) {
    case 0: return 100;
    case 1: score = 10; break;
    default: break;
  }
  do score++; while (score < 12);
  return score;
}

consteval {
  queue_injection(^^{
    // "int classify_logged(int n)" and the logged body:
    \{instr::signature(^^classify, "classify_logged")}
    \{instr::instrument(definition_tokens_of(^^classify))}
  });
}
```

```text
[trace] line 182: block
[trace] line 184: for ( int i = 0 ; i < n ; ++ i )
[trace] line 185: if ( i % 2 == 0 ) -> then
[trace] line 184: for ( int i = 0 ; i < n ; ++ i )
[trace] line 185: if ( i % 2 == 0 ) -> else
...
[trace] line 186: while ( score > 3 )
[trace] line 188: switch ( score )
[trace] line 190: case 1
[trace] line 193: do
[trace] line 193: do
classify_logged(5) = 12
```

`instr::signature` builds the declarator from `return_type_of`,
`parameters_of`, `type_of`, and `identifier_of`, using `list_builder` for the
parameter list.

The same test replaces a virtual member function.  A derived class injects an
`override` whose body is the logged copy, so calls through the base class are
logged:

```cpp
struct logged_counter : counter {
  consteval {
    queue_injection(^^{
      \{member_signature(^^counter::step)} override
      \{instr::instrument(definition_tokens_of(^^counter::step))}
    });
  }
};
```

### A checked `std::vector` built from its own tokens (`tokens_d.sft.cpp`)

`checked_vector` derives from `std::vector<int>`.  A consteval block walks
`members_of(^^std::vector<int>, access_context::unchecked())` and picks
`operator[]`, `at`, `front`, `back`, `push_back`, and `pop_back`.  For each one,
it injects a function with the same signature.  The body is the libstdc++ body
from `definition_tokens_of`, with a log line and a bounds check inserted after
the opening brace:

```cpp
struct checked_vector : std::vector<int> {
  using std::vector<int>::vector;
  // Private in std::vector; used by the copied bodies.
  using _Alloc_traits = __gnu_cxx::__alloc_traits<std::allocator<int>>;
  consteval {
    for (info m : members_of(^^patch::base, access_context::unchecked()))
      if (patch::wanted(m))
        queue_injection(^^{ \{patch::declarator(m)} \{patch::body(m)} });
  }
};
```

```text
[trace] checked_vector::push_back
[trace] checked_vector::operator[], index 1
...
[trace] checked_vector::operator[], index 10
caught: checked_vector::operator[]: index out of range
[trace] checked_vector::pop_back
caught: checked_vector::pop_back: empty vector
```

The copied bodies use library internals such as `__n`, `_M_impl`,
`_M_range_check`, and `_Alloc_traits`.  The example is therefore specific to
libstdc++ and requires `--set_flag=definition_tokens`, which records
`std::vector`.

## Diagnostics

| Situation | Note attached to "expression must have a constant value" |
| --- | --- |
| The argument is not a token sequence, or it is empty where a token is needed | `invalid reflection for intrinsic metafunction` |
| `token_value_of` on an identifier, keyword, or punctuator | `invalid reflection for intrinsic metafunction` |
| `definition_tokens_of` on an entity whose tokens were not recorded, or that has no definition | `the tokens of the definition of this entity are not available ...` |
| `operator_of` on a token sequence that is not exactly one *operator-function-id* | `invalid reflection for intrinsic metafunction` |

See `tests/tests/reflections/tokens_e.sft.cpp`.

## Limitations

- **No in-place replacement.**  A function body cannot be replaced in place.
  The examples inject a new function or an override in a derived class
  instead.
- **Templates are not substituted.**  The tokens of a template specialization
  are those of the template, with template parameters unsubstituted.
- **Tokens only, no semantics.**  The tokens carry no semantic information.
  A rewriter parses them itself, at the token level.
- **Literal spellings change.**  The text of literals comes from their values,
  as described for `token_spelling_of`.
- **C++-generating back end.**  When a translation unit injects code built
  from reflections, the C++-generating back end (`cpfe-cp`) cannot yet
  reproduce the reflections in the C++ it generates.  This is the same
  limitation that affects the existing `reflections/demo1` test.  The
  `tokens_b` and `tokens_c` tests record that failure as expected, and
  `tokens_d` is skipped in that configuration.

## Tests

| Test | Covers |
| --- | --- |
| `reflections/tokens_a.sft.cpp` | Iteration, kinds, spellings, values, locations; reassembly with interpolation and `list_builder`; editing and injecting; operator names and `operator_of` |
| `reflections/tokens_b.sft.cpp` | `definition_tokens_of` for functions, classes, namespaces, and templates.  Case 1 uses the flag; case 2 uses only the attribute |
| `reflections/tokens_c.sft.cpp` | Logged copies of a free function and of a virtual member function |
| `reflections/tokens_d.sft.cpp` | `checked_vector` built from `std::vector` member tokens |
| `reflections/tokens_e.sft.cpp` | Diagnostics |
