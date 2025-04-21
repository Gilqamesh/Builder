#include "reader2.h"

int main() {
  reader_t reader(cin);

  while (!reader.is_at_end()) {
    expr_t* expr = reader.read();
    assert(expr);
    expr->print(cout);
  }

  return 0;
}

