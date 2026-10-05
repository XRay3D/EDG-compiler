//type:fn
//options_all:--c++26 -tused -A
// P1306R5 -- Expansion statements: diagnostics for ill-formed expansion
// statements ([stmt.expand]).

int z[3];

void bad_body() {
  template for (auto a : z)         // error: not a compound statement
    ;
}

void bad_specifiers() {
  template for (static auto a : z) { }        // error: storage class
  template for (thread_local auto a : z) { }  // error: storage class
  template for (extern auto a : z) { }        // error: storage class
  template for (struct Q { int q; } a : z) { }  // error: type definition
}

void not_expandable() {
  int i = 0;
  template for (auto a : i) { }     // error: not expandable
}

void labels(int x) {
  template for (auto a : z) {
    lab: ;                          // error: label in the body
  }
  switch (x) {
  case 1:
    template for (auto a : z) {
    case 2:                         // error: case label of an outer switch
      break;
    default:                        // error: case label of an outer switch
      break;
    }
  }
}

struct not_constant {
  const int *b, *e;
  constexpr const int *begin() const { return b; }
  constexpr const int *end() const { return e; }
};

void not_a_constant_expression(const int *p) {
  template for (auto a : not_constant{p, p + 1}) { }  // error: not constant
}

//paper: P1306R5
//title: Expansion statements
//meeting: Sofia 6/25
