#include "parser.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::SELF_EVALUATING: return "SELF_EVALUATING";
  case expr_type_t::VARIABLE: return "VARIABLE";
  case expr_type_t::QUOTED: return "QUOTED";
  case expr_type_t::ASSIGNMENT: return "ASSIGNMENT";
  case expr_type_t::DEFINITION: return "DEFINITION";
  case expr_type_t::IF: return "IF";
  case expr_type_t::LAMBDA: return "LAMBDA";
  case expr_type_t::BEGIN: return "BEGIN";
  case expr_type_t::APPLICATION: return "APPLICATION";
  default: assert(0);
  }
  return 0;
}

expr_t::expr_t(expr_type_t type, token_t token):
  type(type),
  token(token) {
}

string expr_t::to_string() const {
  return token.to_string();
}

void expr_t::print(ostream& os, const string& prefix, bool is_last) {
  os << prefix << (is_last ? "└── " : "├── ") << expr_type_to_str(type) << ": " << token.to_string() << endl;
  string new_prefix = prefix + (is_last ? "    " : "│   ");

  switch (type) {
  case expr_type_t::SELF_EVALUATING: {
    ((expr_self_evaluating_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::VARIABLE: {
    ((expr_identifier_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::QUOTED: {
    ((expr_quoted_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::ASSIGNMENT: {
    ((expr_assignment_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::DEFINITION: {
    ((expr_define_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::IF: {
    ((expr_if_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::LAMBDA: {
    ((expr_lambda_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::BEGIN: {
    ((expr_begin_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::APPLICATION: {
    ((expr_application_t*)this)->print(os, new_prefix, is_last);
  } break ;
  default: throw token_exception_t("unexpected expr type", token);
  }
}

ostream& operator<<(ostream& os, expr_t* expr) {
  switch (expr->type) {
  case expr_type_t::SELF_EVALUATING: {
    os << (expr_self_evaluating_t*)expr;
  } break ;
  case expr_type_t::VARIABLE: {
    os << (expr_identifier_t*)expr;
  } break ;
  case expr_type_t::QUOTED: {
    os << (expr_quoted_t*)expr;
  } break ;
  case expr_type_t::ASSIGNMENT: {
    os << (expr_assignment_t*)expr;
  } break ;
  case expr_type_t::DEFINITION: {
    os << (expr_define_t*)expr;
  } break ;
  case expr_type_t::IF: {
    os << (expr_if_t*)expr;
  } break ;
  case expr_type_t::LAMBDA: {
    os << (expr_lambda_t*)expr;
  } break ;
  case expr_type_t::BEGIN: {
    os << (expr_begin_t*)expr;
  } break ;
  case expr_type_t::APPLICATION: {
    os << (expr_application_t*)expr;
  } break ;
  default: throw token_exception_t("unexpected expr type", expr->token);
  }

  return os;
}

expr_self_evaluating_t::expr_self_evaluating_t(token_t token):
  base(expr_type_t::SELF_EVALUATING, token)
{
}

void expr_self_evaluating_t::print(ostream& os, const string& prefix, bool is_last) {
}

ostream& operator<<(ostream& os, expr_self_evaluating_t* expr) {
  os << expr->base.token;

  return os;
}

expr_identifier_t::expr_identifier_t(token_t token):
  base(expr_type_t::VARIABLE, token)
{
}

void expr_identifier_t::print(ostream& os, const string& prefix, bool is_last) {
}

ostream& operator<<(ostream& os, expr_identifier_t* expr) {
  os << expr->base.token;

  return os;
}

expr_quoted_t::expr_quoted_t(token_t token, expr_t* quoted_expr):
  base(expr_type_t::QUOTED, token),
  quoted_expr(quoted_expr)
{
}

void expr_quoted_t::print(ostream& os, const string& prefix, bool is_last) {
  quoted_expr->print(os, prefix, true);
}

ostream& operator<<(ostream& os, expr_quoted_t* expr) {
  os << "'" << expr->quoted_expr;

  return os;
}

expr_assignment_t::expr_assignment_t(token_t token, expr_t* lvalue, expr_t* new_value):
  base(expr_type_t::ASSIGNMENT, token),
  lvalue(lvalue),
  new_value(new_value)
{
}

void expr_assignment_t::print(ostream& os, const string& prefix, bool is_last) {
  lvalue->print(os, prefix, false);
  new_value->print(os, prefix, true);
}

ostream& operator<<(ostream& os, expr_assignment_t* expr) {
  os << "(set! " << expr->lvalue << " " << expr->new_value << ")";

  return os;
}

expr_define_t::expr_define_t(token_t token, expr_t* variable, expr_t* value):
  base(expr_type_t::DEFINITION, token),
  variable(variable),
  value(value)
{
}

void expr_define_t::print(ostream& os, const string& prefix, bool is_last) {
  variable->print(os, prefix, false);
  value->print(os, prefix, true);
}

ostream& operator<<(ostream& os, expr_define_t* expr) {
  os << "(define " << expr->variable << " " << expr->value << ")";

  return os;
}

expr_if_t::expr_if_t(token_t token, expr_t* condition, expr_t* consequence, expr_t* alternative):
  base(expr_type_t::IF, token),
  condition(condition),
  consequence(consequence),
  alternative(alternative)
{
}

void expr_if_t::print(ostream& os, const string& prefix, bool is_last) {
  condition->print(os, prefix, false);
  os << prefix << "then:" << endl;
  consequence->print(os, prefix, alternative == 0);
  if (alternative) {
    os << prefix << "else:" << endl;
    alternative->print(os, prefix, true);
  }
}

ostream& operator<<(ostream& os, expr_if_t* expr) {
  os << "(if " << expr->condition << " " << expr->consequence;
  if (expr->alternative) {
    os << " " << expr->alternative;
  }
  os << ")";

  return os;
}

expr_lambda_t::expr_lambda_t(token_t token, const vector<expr_t*>& parameters, const vector<expr_t*>& body):
  base(expr_type_t::LAMBDA, token),
  parameters(parameters),
  body(body)
{
}

void expr_lambda_t::print(ostream& os, const string& prefix, bool is_last) {
  os << prefix << "parameters:" << endl;
  for (int i = 0; i < parameters.size(); ++i) {
    parameters[i]->print(os, prefix, false);
  }
  os << prefix << "body:" << endl;
  for (int i = 0; i < body.size(); ++i) {
    body[i]->print(os, prefix, i + 1 == body.size());
  }
}

ostream& operator<<(ostream& os, expr_lambda_t* expr) {
  os << "(lambda (";
  for (int i = 0; i < expr->parameters.size(); ++i) {
    os << expr->parameters[i];
    if (i + 1 < expr->parameters.size()) {
      os << " ";
    }
  }
  os << ") ";

  for (int i = 0; i < expr->body.size(); ++i) {
    os << expr->body[i];
    if (i + 1 < expr->body.size()) {
      os << " ";
    }
  }

  os << ")";

  return os;
}

expr_begin_t::expr_begin_t(token_t token, const vector<expr_t*>& expressions):
  base(expr_type_t::BEGIN, token),
  expressions(expressions)
{
}

void expr_begin_t::print(ostream& os, const string& prefix, bool is_last) {
  for (int i = 0; i < expressions.size(); ++i) {
    expressions[i]->print(os, prefix, i + 1 == expressions.size());
  }
}

ostream& operator<<(ostream& os, expr_begin_t* expr) {
  os << "(begin ";
  for (int i = 0; i < expr->expressions.size(); ++i) {
    os << expr->expressions[i];
    if (i + 1 < expr->expressions.size()) {
      os << " ";
    }
  }
  os << ")";

  return os;
}

expr_application_t::expr_application_t(token_t token, expr_t* fn_to_apply, const vector<expr_t*>& operands):
  base(expr_type_t::APPLICATION, token),
  fn_to_apply(fn_to_apply),
  operands(operands)
{
}

void expr_application_t::print(ostream& os, const string& prefix, bool is_last) {
  os << prefix << "operand:" << endl;
  fn_to_apply->print(os, prefix, false);
  os << prefix << "args:" << endl;
  for (int i = 0; i < operands.size(); ++i) {
    operands[i]->print(os, prefix, i + 1 == operands.size());
  }
}

ostream& operator<<(ostream& os, expr_application_t* expr) {
  os << "(" << expr->fn_to_apply << " ";
  for (int i = 0; i < expr->operands.size(); ++i) {
    os << expr->operands[i];
    if (i + 1 < expr->operands.size()) {
      os << " ";
    }
  }
  os << ")";

  return os;
}

parser_t::parser_t(const char* source):
  lexer(source) {
}

expr_t* parser_t::eat_expr() {
  switch (peak_token()) {
  case token_type_t::NIL:
  case token_type_t::NUMBER:
  case token_type_t::STRING: return eat_self_evaluating();
  case token_type_t::LEFT_PAREN: {
    eat_token();
    expr_t* expr = 0;
    switch (peak_token()) {
      case token_type_t::DEFINE: {
        expr = eat_define();
      } break ;
      case token_type_t::LAMBDA: {
        expr = eat_lambda();
      } break ;
      case token_type_t::IF: {
        expr = eat_if();
      } break ;
      case token_type_t::WHEN: {
        expr = eat_when();
      } break ;
      case token_type_t::COND: {
        expr = eat_cond();
      } break ;
      case token_type_t::LET: {
        expr = eat_let();
      } break ;
      case token_type_t::BEGIN: {
        expr = eat_begin();
      } break ;
      case token_type_t::QUOTE: {
        expr = eat_quote();
      } break ;
      case token_type_t::SET: {
        expr = eat_set();
      } break ;
      case token_type_t::CONS: {
        expr = eat_cons();
      } break ;
      case token_type_t::CAR: {
        expr = eat_car();
      } break ;
      case token_type_t::CDR: {
        expr = eat_cdr();
      } break ;
      case token_type_t::LIST: {
        expr = eat_list();
      } break ;
      default: {
        expr = eat_application();
      } break ;
    }
    eat_token_error(token_type_t::RIGHT_PAREN);
    return expr;
  } break ;
  case token_type_t::IDENTIFIER: return eat_identifier();
  case token_type_t::LIST:
  case token_type_t::CAR:
  case token_type_t::CDR:
  case token_type_t::CONS: return (expr_t*) new expr_identifier_t(eat_token());
  case token_type_t::APOSTROPHE: return eat_apostrophe();
  case token_type_t::ERROR: throw token_exception_t("error", eat_token());
  case token_type_t::END_OF_FILE: return 0;
  default: throw token_exception_t("unexpected token", eat_token());
  }
  return 0;
}

token_type_t parser_t::peak_token(int ahead) {
  if (tokens.size() <= ahead) {
    fill_tokens();
  }
  if (tokens.size() <= ahead) {
    return token_type_t::END_OF_FILE;
  }
  return tokens[ahead].type;
}

void parser_t::peak_token_error(token_type_t token_type, int ahead) {
  if (peak_token(ahead) != token_type) {
    throw token_exception_t(string("expect token '") + token_type_to_str(token_type) + "'", eat_token());
  }
}

token_t parser_t::eat_token() {
  if (tokens.empty()) {
    fill_tokens();
  }
  assert(!tokens.empty());
  token_t result = tokens.front();
  tokens.pop_front();
  if (result.type == token_type_t::ERROR) {
    throw token_exception_t("unexpected error token", result);
  }
  last_ate = result;
  return result;
}

bool parser_t::eat_token_if(token_type_t token_type) {
  if (peak_token() == token_type) {
    eat_token();
    return true;
  }
  return false;
}

token_t parser_t::eat_token_error(token_type_t token_type) {
  token_t token = eat_token();
  if (token.type != token_type) {
    throw token_exception_t(string("expect token type '") + token_type_to_str(token_type) + "'", token);
  }
  return token;
}

token_t parser_t::ate_token() {
  return last_ate;
}

bool parser_t::is_at_end() {
  if (tokens.empty()) {
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

expr_t* parser_t::eat_self_evaluating() {
  token_t token = eat_token();
  switch (token.type) {
  case token_type_t::NIL:
  case token_type_t::STRING:
  case token_type_t::NUMBER: return (expr_t*) new expr_self_evaluating_t(token);
  default: throw token_exception_t(string("expect token '") + token_type_to_str(token_type_t::NIL) + "' or '" + token_type_to_str(token_type_t::STRING) + "' or '" + token_type_to_str(token_type_t::STRING) + "'", token);
  }
  return 0;
}

expr_t* parser_t::eat_define() {
  token_t define_token = eat_token_error(token_type_t::DEFINE);
  token_t token = eat_token();
  switch (token.type) {
  case token_type_t::IDENTIFIER: {
    // (define ident expr)
    return (expr_t*) new expr_define_t(define_token, (expr_t*) new expr_identifier_t(token), eat_expr());
  } break ;
  case token_type_t::LEFT_PAREN: {
    // (define (ident [ident]*) [expr]*)
    expr_t* var = eat_identifier();
    vector<expr_t*> parameters = eat_identifiers();
    eat_token_error(token_type_t::RIGHT_PAREN);
    return (expr_t*) new expr_define_t(define_token, var, (expr_t*) new expr_lambda_t(token, parameters, eat_while_not(token_type_t::RIGHT_PAREN)));
  } break ;
  default: throw token_exception_t(string("expect token '") + token_type_to_str(token_type_t::IDENTIFIER) + "' or '" + token_type_to_str(token_type_t::LEFT_PAREN) + "'", token);
  }

  assert(0);
  return 0;
}

expr_t* parser_t::eat_lambda() {
  // (lambda ([ident]+) expr)
  token_t lambda_token = eat_token_error(token_type_t::LAMBDA);
  eat_token_error(token_type_t::LEFT_PAREN);
  vector<expr_t*> parameters = eat_identifiers();
  eat_token_error(token_type_t::RIGHT_PAREN);
  return (expr_t*) new expr_lambda_t(lambda_token, parameters, eat_while_not(token_type_t::RIGHT_PAREN));
}

expr_t* parser_t::eat_if() {
  // (if expr expr [expr])
  token_t if_token = eat_token_error(token_type_t::IF);
  expr_t* cond = eat_expr();
  expr_t* consequence = eat_expr();
  expr_t* alternative = 0;
  if (peak_token() != token_type_t::RIGHT_PAREN) {
    alternative = eat_expr();
  }
  return (expr_t*) new expr_if_t(if_token, cond, consequence, alternative);
}

expr_t* parser_t::eat_when() {
  // (when expr [expr]*)
  token_t when_token = eat_token_error(token_type_t::WHEN);

  return (expr_t*) new expr_if_t(when_token, eat_expr(), (expr_t*) new expr_begin_t(when_token, eat_while_not(token_type_t::RIGHT_PAREN)));
}

expr_t* parser_t::eat_cond_branch() {
  if (peak_token() == token_type_t::RIGHT_PAREN) {
    return 0;
  }

  token_t left_paren_token = eat_token_error(token_type_t::LEFT_PAREN);
  if (eat_token_if(token_type_t::ELSE)) {
    expr_t* else_expr = eat_expr();
    eat_token_error(token_type_t::RIGHT_PAREN);
    peak_token_error(token_type_t::RIGHT_PAREN);
    return else_expr;
  }

  expr_t* condition = eat_expr();
  expr_t* consequence = eat_expr();
  eat_token_error(token_type_t::RIGHT_PAREN);
  return (expr_t*) new expr_if_t(left_paren_token, condition, consequence, eat_cond_branch());
}

expr_t* parser_t::eat_cond() {
  // (cond [(expr expr)]* [(else expr)])
  eat_token_error(token_type_t::COND);
  expr_t* result = eat_cond_branch();
  if (!result) {
    throw token_exception_t("expect at least 1 cond branch", ate_token());
  }
  return result;
}

expr_t* parser_t::eat_let() {
  // (let ([(ident expr)]*) expr) -> ((lambda ([ident]*) expr) [expr]*)
  token_t let_token = eat_token_error(token_type_t::LET);
  eat_token_error(token_type_t::LEFT_PAREN);
  vector<expr_t*> application_operands;
  vector<expr_t*> lambda_parameters;
  while (!is_at_end() && eat_token_if(token_type_t::LEFT_PAREN)) {
    lambda_parameters.push_back(eat_identifier());
    application_operands.push_back(eat_expr());
    eat_token_error(token_type_t::RIGHT_PAREN);
  }
  eat_token_error(token_type_t::RIGHT_PAREN);
  return (expr_t*) new expr_application_t(let_token, (expr_t*) new expr_lambda_t(let_token, lambda_parameters, eat_while_not(token_type_t::RIGHT_PAREN)), application_operands);
}

expr_t* parser_t::eat_begin() {
  // (begin [expr]+)
  return (expr_t*) new expr_begin_t(eat_token_error(token_type_t::BEGIN), eat_while_not(token_type_t::RIGHT_PAREN));
}

expr_t* parser_t::eat_quote() {
  // (quote expr)
  return 0;
}

expr_t* parser_t::eat_apostrophe() {
  // 'expr -> (list expr)
  token_t quote_token = eat_token_error(token_type_t::APOSTROPHE);
  if (eat_token_if(token_type_t::LEFT_PAREN)) {
    vector<expr_t*> list_elements = eat_while_not(token_type_t::RIGHT_PAREN);
    eat_token_error(token_type_t::RIGHT_PAREN);
    return (expr_t*) new expr_quoted_t(quote_token, (expr_t*) new expr_list_t(quote_token, list_elements));
  } else {
    return (expr_t*) new expr_quoted_t(quote_token, eat_expr());
  }
}

expr_t* parser_t::eat_set() {
  // (set! ident expr)
  return (expr_t*) new expr_assignment_t(eat_token_error(token_type_t::SET), eat_identifier(), eat_expr());
}

expr_t* parser_t::eat_identifier() {
  // ident
  return (expr_t*) new expr_identifier_t(eat_token_error(token_type_t::IDENTIFIER));
}

expr_t* parser_t::eat_application() {
  // (expr [expr]*)
  return (expr_t*) new expr_application_t(ate_token(), eat_expr(), eat_while_not(token_type_t::RIGHT_PAREN));
}

vector<expr_t*> parser_t::eat_identifiers() {
  vector<expr_t*> identifiers;
  while (!is_at_end() && peak_token() == token_type_t::IDENTIFIER) {
    identifiers.push_back(eat_identifier());
  }
  return identifiers;
}

vector<expr_t*> parser_t::eat_while_not(token_type_t token_type) {
  vector<expr_t*> expressions;
  while (!is_at_end() && peak_token() != token_type) {
    expressions.push_back(eat_expr());
  }
  return expressions;
}
