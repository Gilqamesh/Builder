#ifndef PARSER_H
# define PARSER_H

# include "lexer.h"

enum class expr_type_t : int {
  NIL,
  NUMBER,
  STRING,
  SYMBOL,
  PRIMITIVE_PROC,
  PAIR
};

const char* expr_type_to_str(expr_type_t expr_type);

struct expr_t {
  expr_t(expr_type_t type, token_t token);

  expr_type_t type;
  token_t token;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_nil_t {
  expr_nil_t(token_t token);

  expr_t base;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_number_t {
  expr_number_t(token_t token, double number);

  expr_t base;
  double number;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_string_t {
  expr_string_t(token_t token, const string& str);

  expr_t base;
  string str;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_symbol_t {
  expr_symbol_t(token_t token, string symbol);

  expr_t base;
  string symbol;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_primitive_proc_t {
  expr_primitive_proc_t(token_t token, const function<expr_t*(expr_t*)>& proc);

  expr_t base;
  function<expr_t*(expr_t*)> proc;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_pair_t {
  expr_pair_t(token_t token, expr_t* first, expr_t* second);

  expr_t base;
  expr_t* first;
  expr_t* second;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct parser_t {
  parser_t(const char* source);

  expr_t* eat_expr();

private:
  lexer_t lexer;
  token_t prev_token;
  token_t cur_token;
  token_t next_token;

  token_t peak_token();
  token_t eat_token();
  token_t eat_token_error(token_type_t token_type);
  bool is_at_end();

  expr_t* eat_nil();
  expr_t* eat_number();
  expr_t* eat_string();
  expr_t* eat_symbol();
  expr_t* eat_apostrophe();
  expr_t* eat_list();
};

#endif // PARSER_H

