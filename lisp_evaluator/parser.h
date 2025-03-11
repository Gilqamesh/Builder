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
  COND,
  APPLICATION,
  LIST
};

struct expr_t {
  expr_t() = default;
  expr_t(expr_type_t type, token_t token);

  expr_type_t type;
  token_t token;
  expr_t* next;
};

struct expr_define_t {
  expr_define_t(token_t token);

  expr_t base;
  expr_t* variable;
  expr_t* value;
};

struct expr_lambda_t {
  expr_lambda_t(token_t token);

  expr_t base;
  expr_t* parameters;
  expr_t* body;
};

struct expr_variable_t {
  expr_variable_t(token_t token);

  expr_t base;
};

struct parser_t {
  parser_t(const char* source);

  expr_t* eat_expr();

private:
  lexer_t lexer;

  token_t last_ate;
  deque<token_t> tokens; // in case we want to support look ahead in the future

  using parsers_t = expr_t* (parser_t::* const [token_type_t::_SIZE])();
  expr_t* dispatch_parser(parsers_t parsers);
  parsers_t parsers_expr;
  parsers_t parsers_compound;
  parsers_t parsers_define;

  token_type_t peak_token(int ahead = 0) const;
  token_t eat_token();
  token_type_t eat_token_error(int accepted_types_mask);
  token_t ate_token();
  bool is_at_end() const;
  void fill_tokens(int n = 16);

  //

  const char* start;
  const char* end;
  int         line_start;
  int         line_end;
  int         col_start;
  int         col_end;

  char peak_char(int ahead = 0) const;
  char eat_char();
  bool is_at_end() const;
  void skip_whitespaces();
  bool is_whitespace(char c) const;

  token_t make_token(token_type_t type);
  token_t make_error_token(const char* err_msg, int line, int col);
  token_t make_number(bool dotted);
  token_t make_identifier_if(const char* match, token_type_t type);
  token_t make_identifier();
  token_t make_comment();
  token_t make_string();
}
};

#endif // PARSER_H

