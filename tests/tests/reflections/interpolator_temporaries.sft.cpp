//type:fp
//use_system_includes: true
//edg_header_pack: exp_meta
//options_all:--set_flag=injection

/*
A temporary object created in the operand of an interpolator of a token
sequence is destroyed at the end of the full-expression that contains the
token sequence, like any other temporary ([class.temporary]), and in a return
statement before the local variables of the function ([stmt.return]).  The
pseudo-token holds a copy of the value, so what happens to the token sequence
afterwards (it is returned, assigned, injected) does not matter.

The interpreter used to end a full-expression after evaluating each operand,
which destroyed such a temporary early and then again at the end of the real
full-expression.  The checks below go through constexpr variables and
injected declarations rather than the operands of a comparison in
static_assert, for which the early destruction was skipped.
*/

#include <experimental/meta>
#include <vector>

using namespace std::meta;

// Counts its destructions.
struct Counted {
  int *count;
  constexpr int value() const { return 7; }
  constexpr ~Counted() { ++*count; }
};

// Its destructor reads a member and the local variable it points to.
struct Reader {
  int *p;
  constexpr int value() const { return *p; }
  constexpr ~Reader() { (void)*p; }
};

// Owns dynamic storage, like std::vector.
struct Owner {
  int *p;
  constexpr int value() const { return *p; }
  constexpr ~Owner() { delete p; }
};

// Destroyed at the end of the full-expression: not before the comma operator
// has read the count, and exactly once.
consteval int count_at_comma() {
  int n = 0;
  int seen = (^^{ \val(Counted{&n}.value()) }, n);
  return seen;
}
constexpr int at_comma = count_at_comma();
static_assert(at_comma == 0);

consteval int count_after_statement() {
  int n = 0;
  (void)^^{ \val(Counted{&n}.value()) };
  return n;
}
constexpr int after_statement = count_after_statement();
static_assert(after_statement == 1);

// The pseudo-token holds a copy of the value.
consteval info value_copied() {
  int k = 7;
  info seq = ^^{ \val(k) };
  k = 9;
  return seq;
}
consteval { queue_injection(^^{ constexpr int copied = \{value_copied()}; }); }
static_assert(copied == 7);

// The token sequence is returned, directly or through a local variable or a
// reference parameter, while the destructor reads a local variable of the
// function.
consteval info returned() {
  int k = 1;
  return ^^{ \val(Reader{&k}.value()) };
}
consteval info returned_via_local() {
  int k = 2;
  info seq = ^^{ \val(Reader{&k}.value()) };
  return seq;
}
consteval void store(info &out) {
  int k = 3;
  out = ^^{ \val(Reader{&k}.value()) };
}
consteval info through_reference() {
  info seq = ^^{};
  store(seq);
  return seq;
}
consteval {
  queue_injection(^^{ constexpr int from_return = \{returned()}; });
  queue_injection(^^{ constexpr int from_local = \{returned_via_local()}; });
  queue_injection(^^{ constexpr int from_reference =
                        \{through_reference()}; });
}
static_assert(from_return == 1);
static_assert(from_local == 2);
static_assert(from_reference == 3);

// Temporaries that own dynamic storage, in every kind of context.
consteval info owner_returned() {
  return ^^{ \val(Owner{new int(4)}.value()) };
}
constexpr info owner_at_namespace_scope =
  ^^{ \val(Owner{new int(5)}.value()) };
consteval { (void)^^{ \val(Owner{new int(6)}.value()) }; }
consteval {
  queue_injection(^^{ constexpr int owned_4 = \{owner_returned()}; });
  queue_injection(^^{ constexpr int owned_5 = \{owner_at_namespace_scope}; });
  queue_injection(^^{ constexpr int owned_8 =
                        \val(Owner{new int(8)}.value()); });
}
static_assert(owned_4 == 4);
static_assert(owned_5 == 5);
static_assert(owned_8 == 8);

// The operand of a token interpolator, and std::vector temporaries.
consteval std::vector<info> sequences() { return {^^{ 9 }}; }
consteval std::vector<int> numbers() { return {10, 11}; }
consteval info from_vector_elements() {
  return ^^{ \{sequences()[0]} + \val(numbers()[1]) };
}
consteval {
  queue_injection(^^{ constexpr int sum = \{from_vector_elements()}; });
}
static_assert(sum == 20);
