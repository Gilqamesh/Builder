#include "lexer.h"

int main() {
  lexer_t lexer(cin);
  while (1) {
    token_t token = lexer.eat_token();
    //cout << token.to_string() << endl;
    cout << token << endl;
    if (token.type == token_type_t::END_OF_FILE) {
      break ;
    }
  }
  return 0;
}

