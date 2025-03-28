#ifndef PARSER_H
# define PARSER_H

# include "lexer.h"

enum class expr_type_t : int {
  SELF_EVALUATING,
  VARIABLE,
  QUOTED,
  ASSIGNMENT,
  DEFINITION,
  IF,
  LAMBDA,
  BEGIN,
  APPLICATION
};

const char* expr_type_to_str(expr_type_t expr_type);

struct expr_t {
  expr_t(expr_type_t type, token_t token);

  expr_type_t type;
  token_t token;

  string to_string() const;

  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);

  friend ostream& operator<<(ostream& os, expr_t* expr);
};

struct expr_self_evaluating_t {
  expr_self_evaluating_t(token_t token);

  expr_t base;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_self_evaluating_t* expr);
};

struct expr_identifier_t {
  expr_identifier_t(token_t token);

  expr_t base;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_identifier_t* expr);
};

struct expr_quoted_t {
  expr_quoted_t(token_t token, expr_t* quoted_expr);

  expr_t base;
  expr_t* quoted_expr;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_quoted_t* expr);
};

struct expr_assignment_t {
  expr_assignment_t(token_t token, expr_t* lvalue, expr_t* new_value);

  expr_t base;
  expr_t* lvalue;
  expr_t* new_value;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_assignment_t* expr);
};

struct expr_define_t {
  expr_define_t(token_t token, expr_t* variable, expr_t* value);

  expr_t base;
  expr_t* variable;
  expr_t* value;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_define_t* expr);
};

struct expr_if_t {
  expr_if_t(token_t token, expr_t* condition, expr_t* consequence, expr_t* alternative = 0);

  expr_t base;
  expr_t* condition;
  expr_t* consequence;
  expr_t* alternative;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_if_t* expr);
};

struct expr_lambda_t {
  expr_lambda_t(token_t token, const vector<expr_t*>& parameters, const vector<expr_t*>& body);

  expr_t base;
  vector<expr_t*> parameters;
  vector<expr_t*> body;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_lambda_t* expr);
};

struct expr_begin_t {
  expr_begin_t(token_t token, const vector<expr_t*>& expressions);

  expr_t base;
  vector<expr_t*> expressions;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_begin_t* expr);
};

struct expr_application_t {
  expr_application_t(token_t token, expr_t* fn_to_apply, const vector<expr_t*>& operands);

  expr_t base;
  expr_t* fn_to_apply;
  vector<expr_t*> operands;

  void print(ostream& os, const string& prefix, bool is_last);

  friend ostream& operator<<(ostream& os, expr_application_t* expr);
};

struct parser_t {
  parser_t(const char* source);

  expr_t* eat_expr();

private:
  lexer_t lexer;

  token_t last_ate;
  deque<token_t> tokens; // in case we want to support look ahead in the future

  token_type_t peak_token(int ahead = 0);
  void peak_token_error(token_type_t token_type, int ahead = 0);
  token_t eat_token();
  bool eat_token_if(token_type_t token_type);
  token_t eat_token_error(token_type_t token_type);
  token_t ate_token();
  bool is_at_end();
  void fill_tokens(int n = 16);

  expr_t* eat_self_evaluating();
  expr_t* eat_define();
  expr_t* eat_lambda();
  expr_t* eat_if();
  expr_t* eat_when();
  expr_t* eat_cond_branch();
  expr_t* eat_cond();
  expr_t* eat_let();
  expr_t* eat_begin();
  expr_t* eat_apostrophe();
  expr_t* eat_set();
  expr_t* eat_identifier();
  expr_t* eat_application();

  vector<expr_t*> eat_identifiers();
  vector<expr_t*> eat_while_not(token_type_t token_type);
};

#endif // PARSER_H

