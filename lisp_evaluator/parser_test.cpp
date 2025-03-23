#include "parser.h"

int main() {
  string line;
  string buf;
  while (getline(cin, line)) {
    buf += line + '\n';
  }
  parser_t parser(buf.c_str());
  while (1) {
    try {
      expr_t* expr = parser.eat_expr();
      if (!expr) {
        break ;
      }
      expr->print();
    } catch (token_exception_t& e) {
      cout << e.what() << " " << e.token << endl;
      break ;
    }
  }
  return 0;
}

