#include "parser.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::NIL: return "NIL";
  case expr_type_t::NUMBER: return "NUMBER";
  case expr_type_t::STRING: return "STRING";
  case expr_type_t::SYMBOL: return "SYMBOL";
  case expr_type_t::LIST: return "LIST";
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
  case expr_type_t::NIL: {
    ((expr_nil_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::NUMBER: {
    ((expr_number_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::STRING: {
    ((expr_string_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::SYMBOL: {
    ((expr_symbol_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::LIST: {
    ((expr_list_t*)this)->print(os, new_prefix, is_last);
  } break ;
  default: assert(0);
  }
}

ostream& operator<<(ostream& os, expr_t* expr) {
  switch (expr->type) {
  case expr_type_t::NIL: {
    os << (expr_nil_t*)expr;
  } break ;
  case expr_type_t::NUMBER: {
    os << (expr_number_t*)expr;
  } break ;
  case expr_type_t::STRING: {
    os << (expr_string_t*)expr;
  } break ;
  case expr_type_t::SYMBOL: {
    os << (expr_symbol_t*)expr;
  } break ;
  case expr_type_t::LIST: {
    os << (expr_list_t*)expr;
  } break ;
  default: assert(0);
  }

  return os;
}

expr_nil_t::expr_nil_t(token_t token):
  base(expr_type_t::NIL, token)
{
}

void expr_nil_t::print(ostream& os, const string& prefix, bool is_last) {
}

ostream& operator<<(ostream& os, expr_nil_t* expr_nil) {
  (void) expr_nil;
  os << "nil";

  return os;
}

expr_number_t::expr_number_t(token_t token, double number):
  base(expr_type_t::NUMBER, token),
  number(number)
{
}

void expr_number_t::print(ostream& os, const string& prefix, bool is_last) {
}

ostream& operator<<(ostream& os, expr_number_t* expr_number) {
  os << expr_number->number;

  return os;
}

expr_string_t::expr_string_t(token_t token, const string& str):
  base(expr_type_t::STRING, token),
  str(str)
{
}

void expr_string_t::print(ostream& os, const string& prefix, bool is_last) {
}

ostream& operator<<(ostream& os, expr_string_t* expr_string) {
  os << expr_string->str;

  return os;
}

expr_symbol_t::expr_symbol_t(token_t token, string symbol):
  base(expr_type_t::SYMBOL, token),
  symbol(symbol)
{
}

void expr_symbol_t::print(ostream& os, const string& prefix, bool is_last) {
}

ostream& operator<<(ostream& os, expr_symbol_t* expr_symbol) {
  os << expr_symbol->symbol;

  return os;
}

expr_list_t::expr_list_t(token_t token, const vector<expr_t*>& exprs):
  base(expr_type_t::LIST, token),
  exprs(exprs)
{
}

void expr_list_t::print(ostream& os, const string& prefix, bool is_last) {
  for (int i = 0; i < exprs.size(); ++i) {
    exprs[i]->print(os, prefix, i + 1 == exprs.size());
  }
}

ostream& operator<<(ostream& os, expr_list_t* expr_list) {
  os << "(";
  for (int i = 0; i < expr_list->exprs.size(); ++i) {
    os << expr_list->exprs[i];
    if (i + 1 < expr_list->exprs.size()) {
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
  case token_type_t::NIL: return eat_nil();
  case token_type_t::NUMBER: return eat_number();
  case token_type_t::STRING: return eat_string();
  case token_type_t::APOSTROPHE: return eat_apostrophe();
  case token_type_t::LEFT_PAREN: return eat_list();
  case token_type_t::ERROR: throw token_exception_t("unexpected token", eat_token());
  case token_type_t::END_OF_FILE: return 0;
  default: return eat_symbol();
  }
  return 0;
}

expr_t* parser_t::eat_nil() {
  return (expr_t*) new expr_nil_t(eat_token_error(token_type_t::NIL));
}

expr_t* parser_t::eat_number() {
  token_t token = eat_token_error(token_type_t::NUMBER);
  return (expr_t*) new expr_number_t(token, stod(token.to_string()));
}

expr_t* parser_t::eat_string() {
  token_t token = eat_token_error(token_type_t::STRING);
  return (expr_t*) new expr_string_t(token, token.to_string());
}

expr_t* parser_t::eat_symbol() {
  token_t token = eat_token();
  return (expr_t*) new expr_symbol_t(token, token.to_string());
}

expr_t* parser_t::eat_list() {
  token_t token = eat_token_error(token_type_t::LEFT_PAREN);
  vector<expr_t*> exprs;
  while (!is_at_end() && peak_token() != token_type_t::RIGHT_PAREN) {
    exprs.push_back(eat_expr());
  }
  eat_token_error(token_type_t::RIGHT_PAREN);
  return (expr_t*) new expr_list_t(token, exprs);
}

expr_t* parser_t::eat_apostrophe() {
  return (expr_t*) new expr_list_t(eat_token_error(token_type_t::APOSTROPHE), { (expr_t*) new expr_symbol_t(ate_token(), "quote"), eat_expr() });
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
    if (token.type == token_type_t::COMMENT) {
      ++n;
      continue ;
    }
    tokens.push_back(token);
    if (token.type == token_type_t::END_OF_FILE) {
      return ;
    }
  }
}

