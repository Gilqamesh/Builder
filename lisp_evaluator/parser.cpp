#include "parser.h"

expr_t::expr_t(expr_t* parent, expr_type_t type, token_t token):
  parent(parent),
  type(type),
  token(token),
  err("") {
}

parser_t::parser_t(const char* source):
  lexer(source) {
}

  SELF_EVALUATING, // determined by token
  VARIABLE,
  QUOTED, // determined by token
  ASSIGNMENT,
  DEFINITION, // determined by token
  IF, // determined by token
  LAMBDA, // determined by token
  BEGIN, // determined by token
  COND, // determined by token
  APPLICATION

expr_define_t* eat_define_expr() {
  expr_define_t* define_expr = new expr_define_t(ate_token());
  eat_error_token(token_type_t::IDENTIFIER | token_type_t::LEFT_PAREN);
  switch (eat_token_error(token_type_t::IDENTIFIER | token_type_t::LEFT_PAREN)) {
  case token_type_t::IDENTIFIER: {
    // (define <var> <value>)
    define_expr->variable = make_expr(expr_type_t::VARIABLE, ate_token());
    define_expr->value(eat_expr(define_expr));
  } break ;
  case token_type_t::LEFT_PAREN: {
    // (define (<var> <parameter1> ... <parametern>) <body>)
    if (eat.token().type != token_type_t::IDENTIFIER) {
      throw runtime_error("expect identifier token");
    }
    define_expr->variable = new expr_variable_t(ate_token());
    expr_lambda_t* lambda = new expr_lambda_t();
    define_expr->value = lambda;
    expr_t* parameters = 0;
    expr_t* parameters_cur = 0;
    while (!is_at_end() && eat_token().type == token_type_t::IDENTIFIER) {
      expr_t* parameter = new expr_variable_t(ate_token());
      if (!parameters) {
        parameters = parameter;
      }
      if (parameters_cur) {
        parameters_cur->next = parameter;
      }
      parameters_cur = parameter;
    }
    lambda->parameters = parameters;
    if (ate_token().type != token_type_t::RIGHT_PAREN) {
      throw runtime_error("expect right parentheses token");
    }
    lambda->body = eat_expr();
  } break ;
  }
  if (eat_token().type != token_type_t::RIGHT_PAREN) {
    throw runtime_error("expect right parentheses token");
  }
  return define_expr;
}

expr_lambda_t* eat_lambda_expr() {
  expr_lambda_t* lambda = new expr_lambda_t(ate_token());
  if (eat_token().type != LEFT_PAREN) {
    throw runtime_error("expect left parentheses token");
  }
  expr_t* parameters = 0;
  expr_t* parameters_cur = 0;
  while (!is_at_end() && eat_token().type == token_type_t::IDENTIFIER) {
    expr_t* parameter = new expr_variable_t(ate_token());
    if (!parameters) {
      parameters = parameter;
    }
    if (parameters_cur) {
      parameters_cur->next = parameter;
    }
    parameters_cur = parameter;
  }
  lambda->parameters = parameters;
  if (ate_token().type != token_type_t::RIGHT_PAREN) {
    throw runtime_error("expect right parentheses token");
  }
  lambda->body = eat_expr();
}

// install compound expr
expr_t* parser_t::eat_compound_expr() {
  static expr_t* (parser_t::* const dispatch[sizeof(token_type_t) * 8])() = {
    .token_type_t::DEFINE = &eat_define_expr,
    .token_type_t::LAMBDA = &eat_lambda_expr,
    .token_type_t::IF = &eat_if_expr,
    .token_type_t::ELSE = &eat_else_expr,
    .token_type_t::WHEN = &eat_when_expr,
    .token_type_t::COND = &eat_cond_expr,
    .token_type_t::LET = &eat_let_expr,
    .token_type_t::BEGIN = &eat_begin_expr,
    .token_type_t::QUOTE = &eat_quote_expr,
    .token_type_t::SET = &eat_set_expr,
    .token_type_t::IDENTIFIER = &eat_identifier_expr,
    .token_type_t::CONS = &eat_cons_expr,
    .token_type_t::CAR = &eat_car_expr,
    .token_type_t::CDR = &eat_cdr_expr,
    .token_type_t::LIST = &eat_list_expr,
  };
  token_t token = eat_token();
  const auto f = dispatch[eat_token().type];
  if (f) {
    (this->*f)();
  } else {
    // report error
  }
  switch (eat_token_error()) {
  case token_type_t::DEFINE: return eat_define_expr();
  case token_type_t::LAMBDA: return eat_lambda_expr();
  case token_type_t::IF: return eat_if_expr();
  case token_type_t::ELSE: return eat_else_expr();
  case token_type_t::WHEN: return eat_when_expr();
  case token_type_t::COND: return eat_cond_expr();
  case token_type_t::LET: return eat_let_expr();
  case token_type_t::BEGIN: return eat_begin_expr();
  case token_type_t::QUOTE: return eat_quote_expr();
  case token_type_t::SET: return eat_set_expr();
  case token_type_t::IDENTIFIER: return eat_identifier_expr();
  case token_type_t::CONS: return eat_cons_expr();
  case token_type_t::CAR: return eat_car_expr();
  case token_type_t::CDR: return eat_cdr_expr();
  case token_type_t::LIST: return eat_list_expr();
  default: throw runtime_error("");
  }
}

expr_t* parser_t::dispatch_parser(parsers_t parsers) {
  const parser = parsers[eat_token().type()];
  if (parser) {
    return (this->*parser)();
  } else {
    string error_msg = "expect ";
    int n_options = 0;
    for (int i = 0; i < sizeof(parsers) / sizeof(parsers[0]); ++i) {
      if (parsers[i]) {
        if (n_options++) {
          error_msg += " or ";
        }
        error_msg += token_type_to_str(i);
      }
    }
    throw runtime_error(error_msg);
  }
}

expr_t* parser_t::eat_expr() {
  switch (eat_token().type) {
  case token_type_t::FALSE:
  case token_type_t::NUMBER:
  case token_type_t::STRING: return new expr_t(expr_type_t::SELF_EVALUATING, token);
  case token_type_t::LEFT_PAREN: return eat_compound_expr();
  case token_type_t::IDENTIFIER: return make_expr(expr_type_t:VARIABLE);
  case token_type_t::LIST:
  case token_type_t::CAR:
  case token_type_t::CDR:
  case token_type_t::CONS: return make_expr(expr_type_t::APPLICATION);
  case token_type_t::ERROR: return 0;
  case token_type_t::APOSTROPHE: return eat_quote_expr(parent);
  case token_type_t::END_OF_FILE: return 0;
  }
}

token_type_t parser_t::peak_token(int ahead) const {
  if (tokens.size() <= ahead) {
    fill_tokens();
  }
  if (tokens.size() <= ahead) {
    return token_type_t::END_OF_FILE;
  }
  return tokens[ahead].type;
}

token_t parser_t::eat_token() {
  if (tokens.empty()) {
    fill_tokens();
  }
  assert(!tokens.empty());
  token_t result = tokens.front();
  tokens.pop_front();
  last_ate = result;
  return result;
}

token_type_t parser_t::eat_token_error(int accepted_types_mask) {
  token_t token = eat_token();
  if (token.type & accepted_types_mask) {
    return token.type;
  }
  string err_msg = "expect ";
  int n_accepted_types = 0;
  for (int i = 0; i < sizeof(token_type_t) * 8; ++i) {
    if ((1 << i) & accepted_types_mask) {
      if (n_accepted_types++) {
        err_msg += " or ";
      }
      err_msg += token_type_to_str((token_type_t)(1 << i));
    }
  }
  throw runtime_error(err_msg);
  return token_type_t::ERROR;
}

token_t ate_token() {
  return last_ate;
}

bool parser_t::is_at_end() const {
  if (tokens.empty() {
    fill_tokens();
  }
  assert(!tokens.empty());
  return tokens.front().type == token_type_t::END_OF_FILE;
}

void parser_t::fill_tokens(int n) {
  while (n--) {
    token_t token = lexer.eat_token();
    tokens.push_back(token);
    if (token.type == token_type_t::END_OF_FILE) {
      return ;
    }
  }
}

expr_define_t* parser_t::make_define_expr(expr_t* parent, token_t token) {
  expr_define_t* result = new expr_define_t;
  result->base;
}

expr_t* parser_t::make_expr(expr_t* parent, expr_type_t type, token_t token) {
  return new expr_t {
    .type = type,
    .token = token,
    .error = "",
    .parent = parent
  };
}

expr_t* parser_t::make_error_expr(expr_t* parent, const char* err_msg, token_t token) {
  expr_t* result = make_expr(parent, expr_type_t::ERROR, token);
  result->error = err_msg;
  return result;
}

