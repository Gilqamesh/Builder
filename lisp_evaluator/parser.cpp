#include "parser.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::NIL: return "NIL";
  case expr_type_t::NUMBER: return "NUMBER";
  case expr_type_t::STRING: return "STRING";
  case expr_type_t::SYMBOL: return "SYMBOL";
  case expr_type_t::PRIMITIVE_PROC: return "PRIMITIVE PROC";
  case expr_type_t::PAIR: return "PAIR";
  default: assert(0);
  }
  return 0;
}

expr_t::expr_t(expr_type_t type, token_t token):
  type(type),
  token(token) {
}

string expr_t::to_string() {
  switch (type) {
  case expr_type_t::NIL: {
    return ((expr_nil_t*)this)->to_string();
  } break ;
  case expr_type_t::NUMBER: {
    return ((expr_number_t*)this)->to_string();
  } break ;
  case expr_type_t::STRING: {
    return ((expr_string_t*)this)->to_string();
  } break ;
  case expr_type_t::SYMBOL: {
    return ((expr_symbol_t*)this)->to_string();
  } break ;
  case expr_type_t::PRIMITIVE_PROC: {
    return ((expr_primitive_proc_t*)this)->to_string();
  } break ;
  case expr_type_t::PAIR: {
    return ((expr_pair_t*)this)->to_string();
  } break ;
  default: assert(0);
  }
}

void expr_t::print(ostream& os, const string& prefix, bool is_last) {
  //os << prefix << (is_last ? "└── " : "├── ") << expr_type_to_str(type) << ": " << token.to_string() << endl;
  os << prefix << (is_last ? "└── " : "├── ") << to_string() << endl;
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
  case expr_type_t::PRIMITIVE_PROC: {
    ((expr_primitive_proc_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::PAIR: {
    ((expr_pair_t*)this)->print(os, new_prefix, is_last);
  } break ;
  default: assert(0);
  }
}

expr_nil_t::expr_nil_t(token_t token):
  base(expr_type_t::NIL, token)
{
}

string expr_nil_t::to_string() {
  return "nil";
}

void expr_nil_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_number_t::expr_number_t(token_t token, double number):
  base(expr_type_t::NUMBER, token),
  number(number)
{
}

string expr_number_t::to_string() {
  return std::to_string(number);
}

void expr_number_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_string_t::expr_string_t(token_t token, const string& str):
  base(expr_type_t::STRING, token),
  str(str)
{
}

string expr_string_t::to_string() {
  return str;
}

void expr_string_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_symbol_t::expr_symbol_t(token_t token, string symbol):
  base(expr_type_t::SYMBOL, token),
  symbol(symbol)
{
}

string expr_symbol_t::to_string() {
  return symbol;
}

void expr_symbol_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_primitive_proc_t::expr_primitive_proc_t(token_t token, const function<expr_t*(expr_t*)>& proc):
  base(expr_type_t::PRIMITIVE_PROC, token),
  proc(proc)
{
}

string expr_primitive_proc_t::to_string() {
  return "primitive proc";
}

void expr_primitive_proc_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_pair_t::expr_pair_t(token_t token, expr_t* first, expr_t* second):
  base(expr_type_t::PAIR, token),
  first(first),
  second(second)
{
}

string expr_pair_t::to_string() {
  string result = "(";
  expr_t* cur = (expr_t*)this;
  while (cur->type == expr_type_t::PAIR) {
    expr_pair_t* pair = (expr_pair_t*)cur;
    result += pair->first->to_string();
    
    if (pair->second->type == expr_type_t::PAIR) {
      result += " ";
      cur = pair->second;
    } else if (pair->second->type == expr_type_t::NIL) {
      break ;
    } else {
      result += " . " + pair->second->to_string();
      break ;
    }
  }
  result += ")";

  return result;
}

void expr_pair_t::print(ostream& os, const string& prefix, bool is_last) {
  first->print(os, prefix, false);
  if (second->type == expr_type_t::PAIR) {
    second->print(os, prefix, true);
  } else if (second->type == expr_type_t::NIL) {
    os << prefix << (is_last ? "└── " : "├── ") << "NIL" << endl;
  } else {
    os << prefix << (is_last ? "└── " : "├── ") << second->to_string() << endl;
  }
}

parser_t::parser_t(const char* source):
  lexer(source) {
  eat_token();
}

expr_t* parser_t::eat_expr() {
  switch (peak_token().type) {
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
  token_t left_paren = eat_token_error(token_type_t::LEFT_PAREN);
  expr_t* list_terminator = (expr_t*) new expr_nil_t(left_paren);
  expr_pair_t* first = 0;
  expr_pair_t* prev = 0;
  while (!is_at_end() && peak_token().type != token_type_t::RIGHT_PAREN) {
    expr_t* expr = eat_expr();
    expr_pair_t* cur = new expr_pair_t(expr->token, expr, list_terminator);
    if (!first) {
      first = cur;
    } else {
      prev->second = (expr_t*)cur;
    }
    prev = cur;
  }
  eat_token_error(token_type_t::RIGHT_PAREN);
  if (first) {
    return (expr_t*) first;
  }
  return list_terminator;
}

expr_t* parser_t::eat_apostrophe() {
  return (expr_t*) new expr_pair_t(eat_token_error(token_type_t::APOSTROPHE), (expr_t*) new expr_symbol_t(prev_token, "quote"), eat_expr());
}

token_t parser_t::peak_token() {
  return next_token;
}

token_t parser_t::eat_token() {
  prev_token = cur_token;
  cur_token = next_token;
  next_token = lexer.eat_token();
  while (next_token.type == token_type_t::COMMENT) {
    next_token = lexer.eat_token();
  }
  if (next_token.type == token_type_t::ERROR) {
    throw token_exception_t("unexpected error token", next_token);
  }
  return cur_token;
}

token_t parser_t::eat_token_error(token_type_t token_type) {
  token_t token = eat_token();
  if (token.type != token_type) {
    throw token_exception_t(string("expect token type '") + token_type_to_str(token_type) + "'", token);
  }
  return token;
}

bool parser_t::is_at_end() {
  return peak_token().type == token_type_t::END_OF_FILE;
}

