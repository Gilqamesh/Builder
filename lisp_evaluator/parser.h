#ifndef PARSER_H
# define PARSER_H

# include "lexer.h"

enum class expr_type_t : int {
  NIL,
  NUMBER,
  STRING,
  SYMBOL,
  PAIR
};

const char* expr_type_to_str(expr_type_t expr_type);

struct expr_t {
  expr_t(expr_type_t type, token_t token);

  expr_type_t type;
  token_t token;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
  friend ostream& operator<<(ostream& os, expr_t* expr);
};

struct expr_nil_t {
  expr_nil_t(token_t token);

  expr_t base;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
  friend ostream& operator<<(ostream& os, expr_nil_t* expr);
};

struct expr_number_t {
  expr_number_t(token_t token, double number);

  expr_t base;
  double number;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
  friend ostream& operator<<(ostream& os, expr_number_t* expr);
};

struct expr_string_t {
  expr_string_t(token_t token, const string& str);

  expr_t base;
  string str;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
  friend ostream& operator<<(ostream& os, expr_string_t* expr);
};

struct expr_symbol_t {
  expr_symbol_t(token_t token, string symbol);

  expr_t base;
  string symbol;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
  friend ostream& operator<<(ostream& os, expr_symbol_t* expr);
};

struct expr_pair_t {
  expr_pair_t(token_t token, expr_t* first, expr_t* second);

  expr_t base;
  expr_t* first;
  expr_t* second;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
  friend ostream& operator<<(ostream& os, expr_symbol_t* expr);
};

struct parser_t {
  parser_t(const char* source);

  expr_t* eat_expr();

private:
  lexer_t lexer;

  token_t last_ate;
  deque<token_t> tokens; // in case we want to support look ahead in the future

  token_type_t peak_token(int ahead = 0);
  token_t eat_token();
  token_t eat_token_error(token_type_t token_type);
  token_t ate_token();
  bool is_at_end();
  void fill_tokens(int n = 16);

  expr_t* eat_nil();
  expr_t* eat_number();
  expr_t* eat_string();
  expr_t* eat_symbol();
  expr_t* eat_apostrophe();
  expr_t* eat_list();
};

#endif // PARSER_H

