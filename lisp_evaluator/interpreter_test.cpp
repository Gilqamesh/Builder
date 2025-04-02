#include "interpreter.h"

void repl() {
  interpreter_t interpreter;

  while (1) {
    cout << "> " << flush;
    string line;
    if (!getline(cin, line)) {
      break ;
    }
    lexer_t lexer(line.c_str());
    try {
      if (expr_t* read_expr = interpreter.read(lexer)) {
        if (expr_t* evaled_expr = interpreter.eval(read_expr)) {
          cout << evaled_expr->to_string() << endl;
        } else {
          cerr << "could not eval read expr: '" << read_expr->to_string() << "'" << endl;
        }
      } else {
        cerr << "could not read expr" << endl;
      }
    } catch (const exception& e) {
      cerr << "exception: " << e.what() << endl;
    }
  }
}

int main() {
  repl();

  // string line;
  // string buf;
  // while (getline(cin, line)) {
  //   buf += line + '\n';
  // }
  // interpreter_t interpreter;
  // lexer_t lexer(buf.c_str());
  // while (1) {
  //   try {
  //     expr_t* expr = interpreter.read(lexer);
  //     if (!expr) {
  //       break ;
  //     }
  //     cout << "PRINT" << endl;
  //     expr->print();
  //     cout << "EVAL" << endl;
  //     interpreter.eval(expr)->print();
  //   } catch (token_exception_t& e) {
  //     cout << e.what() << " " << e.token << endl;
  //     break ;
  //   }
  // }
  return 0;
}

