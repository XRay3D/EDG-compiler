//type:rp
//use_system_includes: true
//edg_header_pack: exp_meta
//options_all:--set_flag=injection --set_flag=definition_tokens
//require:DO_IL_LOWERING 1

/*
A vector derived from std::vector<int> whose element access and push/pop
member functions are built from the tokens of the corresponding std::vector
member functions (std::meta::definition_tokens_of), with a log line and a
bounds check inserted at the start of each body.  The copied bodies refer to
implementation details of the library (e.g., the parameter "__n" and
_M_range_check); _Alloc_traits, which is private in std::vector, is
redeclared in the derived class.
*/

#include <experimental/meta>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
using namespace std::meta;

void trace(const char *msg) { std::printf("[trace] %s\n", msg); }
void trace(const char *msg, std::size_t n) {
  std::printf("[trace] %s, index %zu\n", msg, n);
}

namespace patch {
using base = std::vector<int>;

consteval bool wanted(info m) {
  if (!is_function(m) || is_special_member_function(m)) return false;
  if (is_operator_function(m))
    return operator_of(m) == operators::op_square_brackets;
  std::string_view n = identifier_of(m);
  return n == "at" || n == "front" || n == "back" || n == "push_back" ||
         n == "pop_back";
}

consteval std::string name_of(info m) {
  std::string name = "operator[]";
  if (!is_operator_function(m)) name = identifier_of(m);
  return name;
}

consteval bool has_index(info m) {
  for (info p : parameters_of(m))
    if (identifier_of(p) == "__n") return true;
  return false;
}

// "return-type name(parameters) [const]".
consteval info declarator(info m) {
  list_builder params;
  for (info p : parameters_of(m))
    params += ^^{ typename \[: type_of(p) :] \[identifier_of(p)] };
  info name = is_operator_function(m) ? ^^{ operator[] }
                                      : ^^{ \[identifier_of(m)] };
  info d = ^^{ typename \[: return_type_of(m) :] \{name} ( \{params} ) };
  if (is_const(m)) d = ^^{ \{d} const };
  return d;
}

// The std::vector body with a log line and a check inserted after "{".
consteval info body(info m) {
  std::vector<info> t = tokens_of(definition_tokens_of(m));
  std::string name = name_of(m);
  std::string full = "checked_vector::" + name;
  if (is_const(m)) full += " const";
  std::string_view what = std::define_static_string(full);
  info out = t.front();                                     // {
  if (has_index(m))
    out = ^^{ \{out} trace(\str(what), __n); };
  else
    out = ^^{ \{out} trace(\str(what)); };
  if (name == "operator[]") {
    std::string_view msg = std::define_static_string(
      "checked_vector::operator[]: index out of range");
    out = ^^{ \{out}
      if (__n >= this->size()) throw std::out_of_range(\str(msg)); };
  } else if (name == "front" || name == "back" || name == "pop_back") {
    std::string_view msg =
      std::define_static_string("checked_vector::" + name + ": empty vector");
    out = ^^{ \{out} if (this->empty()) throw std::out_of_range(\str(msg)); };
  }
  for (std::size_t k = 1; k < t.size(); ++k) out = ^^{ \{out} \{t[k]} };
  return out;
}
}  // namespace patch

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

int main() {
  checked_vector v{1, 2, 3};
  v.push_back(4);
  std::printf("v[1] = %d, at(2) = %d, front = %d, back = %d\n", v[1], v.at(2),
              v.front(), v.back());
  v.pop_back();
  try {
    std::printf("v[10] = %d\n", v[10]);
  } catch (const std::out_of_range &e) {
    std::printf("caught: %s\n", e.what());
  }
  checked_vector e;
  try {
    e.pop_back();
  } catch (const std::out_of_range &x) {
    std::printf("caught: %s\n", x.what());
  }
}
