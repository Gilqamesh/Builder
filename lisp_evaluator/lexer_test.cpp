#include "lexer.h"

int main() {
  string line;
  string buf;
  while (getline(cin, line)) {
    buf += line + '\n';
  }
  lexer_t lexer(buf.c_str());
  while (1) {
    token_t token = lexer.eat_token();
    cout << token.to_string() << endl;
    if (token.type == token_type_t::END_OF_FILE) {
      break ;
    }
  }
  return 0;
}

