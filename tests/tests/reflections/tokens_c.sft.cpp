//type:rp
//use_system_includes: true
//edg_header_pack: exp_meta
//options_all:--set_flag=injection --set_flag=definition_tokens

/*
Take the tokens of functions written above (std::meta::definition_tokens_of)
and inject copies in which every block, if/else branch, loop iteration,
switch, and case label logs a line; branches and loop bodies without braces
are put in braces to make room for the log line.  The copy of a free
function is a new function; the copy of a virtual member function overrides
it in a derived class, so calls through the base class are logged.
*/

#include <experimental/meta>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

void trace(const char *msg) { std::printf("[trace] %s\n", msg); }

// A token-level rewriter that adds logging to the statements of a function
// body (the token sequence returned by definition_tokens_of).
namespace instr {
using namespace std::meta;

consteval std::string number(unsigned long n) {
  std::string s;
  do { s.insert(s.begin(), char('0' + n % 10)); n /= 10; } while (n != 0);
  return s;
}

struct rewriter {
  std::vector<info> t;
  std::size_t i = 0;
  info out = ^^{};

  consteval std::string_view sp(std::size_t k) const {
    return k < t.size() ? token_spelling_of(t[k]) : std::string_view();
  }
  consteval bool at(std::string_view s) const { return sp(i) == s; }
  consteval std::string line() const {
    std::size_t k = i < t.size() ? i : t.size() - 1;
    return number(token_location_of(t[k]).line());
  }
  consteval void emit(info x) { out = ^^{ \{out} \{x} }; }
  consteval void copy() { emit(t[i++]); }
  consteval void log(std::string_view msg) {
    // The operand of \str must designate static storage.
    std::string_view text = define_static_string(msg);
    emit(^^{ trace(\str(text)); });
  }
  // Copy a balanced group "( ... )" and return its text.
  consteval std::string group() {
    std::string text;
    int depth = 0;
    do {
      if (at("(") || at("[") || at("{")) ++depth;
      else if (at(")") || at("]") || at("}")) --depth;
      if (!text.empty()) text += ' ';
      text += sp(i);
      copy();
    } while (depth > 0 && i < t.size());
    return text;
  }
  // Copy a simple statement (up to and including the ";" at depth 0).
  consteval void simple() {
    int depth = 0;
    while (i < t.size()) {
      if (at("(") || at("[") || at("{")) ++depth;
      else if (at(")") || at("]") || at("}")) --depth;
      bool end = depth == 0 && at(";");
      copy();
      if (end) break;
    }
  }
  // Single "{" and "}" tokens (a token-sequence literal cannot hold an
  // unbalanced brace), taken from the body being rewritten.
  info lbrace, rbrace;

  // A substatement of if/else/while/for/do: always put in braces (so that a
  // substatement without braces can be given a log line), with a log line
  // saying which branch or iteration is executed.
  consteval void braced(std::string msg) {
    emit(lbrace);
    log(msg);
    if (at("{")) {
      ++i;                         // the block's own braces are merged
      while (i < t.size() && !at("}")) statement();
      ++i;
    } else {
      statement();
    }
    emit(rbrace);
  }

  // Log "switch (...)" (the text of the header that starts at token i).
  consteval void log_switch_head(std::string l) {
    std::string head = "switch";
    int depth = 0;
    for (std::size_t k = i + 1; k < t.size(); ++k) {
      if (sp(k) == "(") ++depth;
      head += ' ';
      head += sp(k);
      if (sp(k) == ")" && --depth == 0) break;
    }
    log("line " + l + ": " + head);
  }

  consteval void statement() {
    std::string l = line();
    if (at("{")) {
      copy();
      log("line " + l + ": block");
      while (i < t.size() && !at("}")) statement();
      copy();
    } else if (at("if")) {
      copy();
      std::string cond = group();
      braced("line " + l + ": if " + cond + " -> then");
      if (at("else")) {
        copy();
        braced("line " + l + ": if " + cond + " -> else");
      }
    } else if (at("switch")) {
      // Code before the first case label would be unreachable, so the log
      // line goes before the switch statement.
      log_switch_head(l);
      copy();
      group();
      copy();                      // {
      while (i < t.size() && !at("}")) statement();
      copy();                      // }
    } else if (at("while") || at("for")) {
      std::string kw(sp(i));
      copy();
      std::string head = group();
      braced("line " + l + ": " + kw + " " + head);
    } else if (at("do")) {
      copy();
      braced("line " + l + ": do");
      copy();                      // while
      group();
      copy();                      // ;
    } else if (at("case") || at("default")) {
      std::string label;
      while (i < t.size() && !at(":")) {
        if (!label.empty()) label += ' ';
        label += sp(i);
        copy();
      }
      copy();                      // :
      log("line " + l + ": " + label);
    } else {
      simple();
    }
  }
};

// Return a copy of the body "{ ... }" with logging added.
consteval info instrument(info body) {
  rewriter r;
  r.t = tokens_of(body);
  r.lbrace = r.t.front();
  r.rbrace = r.t.back();
  r.statement();
  return r.out;
}

// The declarator of a copy of function fn named new_name.
consteval info signature(info fn, std::string_view new_name) {
  list_builder params;
  for (info p : parameters_of(fn))
    params += ^^{ typename \[: type_of(p) :] \[identifier_of(p)] };
  return ^^{ typename \[: return_type_of(fn) :] \[new_name] ( \{params} ) };
}
}  // namespace instr

using namespace std::meta;

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

// int classify_logged(int n) { ...the body of classify with logging... }
consteval {
  queue_injection(^^{
    \{instr::signature(^^classify, "classify_logged")}
    \{instr::instrument(definition_tokens_of(^^classify))}
  });
}

struct counter {
  int limit = 3;
  virtual int step(int x) const {
    if (x > limit)
      return x - limit;
    else if (x == limit)
      return 0;
    for (int k = 0; k < 2; ++k) x += k;
    return x;
  }
  virtual ~counter() = default;
};

// "int step(int x) const": the declarator of member function fn.
consteval info member_signature(info fn) {
  list_builder params;
  for (info p : parameters_of(fn))
    params += ^^{ typename \[: type_of(p) :] \[identifier_of(p)] };
  info sig = ^^{ typename \[: return_type_of(fn) :] \[identifier_of(fn)]
                 ( \{params} ) };
  if (is_const(fn)) sig = ^^{ \{sig} const };
  return sig;
}

// Replace counter::step by an override whose body is the logged copy.
struct logged_counter : counter {
  consteval {
    queue_injection(^^{
      \{member_signature(^^counter::step)} override
      \{instr::instrument(definition_tokens_of(^^counter::step))}
    });
  }
};

int main() {
  std::printf("classify(5) = %d\n", classify(5));
  std::printf("classify_logged(5) = %d\n", classify_logged(5));
  logged_counter lc;
  const counter &c = lc;
  int a = c.step(5);
  int b = c.step(1);
  std::printf("step(5) = %d, step(1) = %d (plain: %d, %d)\n", a, b,
              counter().step(5), counter().step(1));
}
