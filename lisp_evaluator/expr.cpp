#include "expr.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::VOID: return "VOID";
  case expr_type_t::NIL: return "NIL";
  case expr_type_t::CHAR: return "CHAR";
  case expr_type_t::INTEGER: return "INTEGER";
  case expr_type_t::REAL: return "REAL";
  case expr_type_t::STRING: return "STRING";
  case expr_type_t::SYMBOL: return "SYMBOL";
  case expr_type_t::ENV: return "ENV";
  case expr_type_t::PRIMITIVE_PROC: return "PRIMITIVE PROC";
  case expr_type_t::SPECIAL_FORM: return "SPECIAL FORM";
  case expr_type_t::MACRO: return "MACRO";
  case expr_type_t::COMPOUND_PROC: return "COMPOUND PROC";
  case expr_type_t::CONS: return "CONS";
  case expr_type_t::ISTREAM: return "ISTREAM";
  case expr_type_t::OSTREAM: return "OSTREAM";
  default: assert(0);
  }
  return 0;
}

expr_t::expr_t(expr_type_t type):
  type(type)
{
}

string expr_t::to_string() {
  switch (type) {
  case expr_type_t::VOID: {
    return ((expr_void_t*)this)->to_string();
  } break ;
  case expr_type_t::NIL: {
    return ((expr_nil_t*)this)->to_string();
  } break ;
  case expr_type_t::CHAR: {
    return ((expr_char_t*)this)->to_string();
  } break ;
  case expr_type_t::INTEGER: {
    return ((expr_integer_t*)this)->to_string();
  } break ;
  case expr_type_t::REAL: {
    return ((expr_real_t*)this)->to_string();
  } break ;
  case expr_type_t::STRING: {
    return ((expr_string_t*)this)->to_string();
  } break ;
  case expr_type_t::SYMBOL: {
    return ((expr_symbol_t*)this)->to_string();
  } break ;
  case expr_type_t::ENV: {
    return ((expr_env_t*)this)->to_string();
  } break ;
  case expr_type_t::PRIMITIVE_PROC: {
    return ((expr_primitive_proc_t*)this)->to_string();
  } break ;
  case expr_type_t::SPECIAL_FORM: {
    return ((expr_special_form_t*)this)->to_string();
  } break ;
  case expr_type_t::MACRO: {
    return ((expr_macro_t*)this)->to_string();
  } break ;
  case expr_type_t::COMPOUND_PROC: {
    return ((expr_compound_proc_t*)this)->to_string();
  } break ;
  case expr_type_t::CONS: {
    return ((expr_cons_t*)this)->to_string();
  } break ;
  case expr_type_t::ISTREAM: {
    return ((expr_istream_t*)this)->to_string();
  } break ;
  case expr_type_t::OSTREAM: {
    return ((expr_ostream_t*)this)->to_string();
  } break ;
  default: assert(0);
  }
}

void expr_t::print(ostream& os, const string& prefix, bool is_last) {
  os << prefix << (is_last ? "└── " : "├── ") << to_string() << endl;
  string new_prefix = prefix + (is_last ? "    " : "│   ");

  switch (type) {
  case expr_type_t::VOID: {
    ((expr_void_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::NIL: {
    ((expr_nil_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::INTEGER: {
    ((expr_integer_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::CHAR: {
    ((expr_char_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::REAL: {
    ((expr_real_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::STRING: {
    ((expr_string_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::SYMBOL: {
    ((expr_symbol_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::ENV: {
    ((expr_env_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::PRIMITIVE_PROC: {
    ((expr_primitive_proc_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::SPECIAL_FORM: {
    ((expr_special_form_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::MACRO: {
    ((expr_macro_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::COMPOUND_PROC: {
    ((expr_compound_proc_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::CONS: {
    ((expr_cons_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::ISTREAM: {
    ((expr_istream_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::OSTREAM: {
    ((expr_ostream_t*)this)->print(os, new_prefix, is_last);
  } break ;
  default: assert(0);
  }
}

expr_void_t::expr_void_t():
  base(expr_type_t::VOID)
{
}

string expr_void_t::to_string() {
  return "";
}

void expr_void_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_nil_t::expr_nil_t():
  base(expr_type_t::NIL)
{
}

string expr_nil_t::to_string() {
  return "nil";
}

void expr_nil_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_char_t::expr_char_t(char c):
  base(expr_type_t::CHAR),
  c(c)
{
}

string expr_char_t::to_string() {
  return "'" + string(1, c) + "'";
}

void expr_char_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_integer_t::expr_integer_t(int64_t integer):
  base(expr_type_t::INTEGER),
  integer(integer)
{
}

string expr_integer_t::to_string() {
  return std::to_string(integer);
}

void expr_integer_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_real_t::expr_real_t(double real):
  base(expr_type_t::REAL),
  real(real)
{
}

string expr_real_t::to_string() {
  return std::to_string(real);
}

void expr_real_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_string_t::expr_string_t(const string& str):
  base(expr_type_t::STRING),
  str(str)
{
}

string expr_string_t::to_string() {
  return str;
}

void expr_string_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_symbol_t::expr_symbol_t(string symbol):
  base(expr_type_t::SYMBOL),
  symbol(symbol)
{
}

string expr_symbol_t::to_string() {
  return symbol;
}

void expr_symbol_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_env_t::expr_env_t():
  base(expr_type_t::ENV),
  parent(0)
{
}

expr_t* expr_env_t::get(expr_t* symbol) {
  if (symbol->type != expr_type_t::SYMBOL) {
    throw expr_exception_t("get: symbol expected", symbol);
  }
  expr_env_t* cur = this;
  while (cur) {
    auto it = cur->bindings.find(symbol);
    if (it != cur->bindings.end()) {
      return it->second;
    }
    cur = cur->parent;
  }
  return 0;
}

expr_t* expr_env_t::set(expr_t* symbol, expr_t* expr) {
  if (symbol->type != expr_type_t::SYMBOL) {
    throw expr_exception_t("set: symbol expected", symbol);
  }
  expr_env_t* cur = this;
  while (cur) {
    auto it = cur->bindings.find(symbol);
    if (it != cur->bindings.end()) {
      it->second = expr;
      return expr;
    }
    cur = cur->parent;
  }
  throw expr_exception_t("set: expr not found", expr);
}

expr_t* expr_env_t::define(expr_t* symbol, expr_t* expr) {
  if (symbol->type != expr_type_t::SYMBOL) {
    throw expr_exception_t("define: symbol expected", symbol);
  }
  bindings[symbol] = expr;
  return expr;
}

string expr_env_t::to_string() {
  return "env";
}

void expr_env_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_primitive_proc_t::expr_primitive_proc_t(const function<expr_t*(expr_t*)>& f, int arity, bool is_variadic):
  base(expr_type_t::PRIMITIVE_PROC),
  f(f),
  arity(arity),
  is_variadic(is_variadic)
{
}

string expr_primitive_proc_t::to_string() {
  string result = "<#procedure (";
  for (int i = 0; i < arity; ++i) {
    result += "_";
    if (i + 1 < arity) {
      result += " ";
    }
  }
  if (is_variadic) {
    result += " . _";
  }
  result += ")>";
  return result;
}

void expr_primitive_proc_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_special_form_t::expr_special_form_t(const function<expr_t*(expr_t*, expr_env_t*)>& f):
  base(expr_type_t::SPECIAL_FORM),
  f(f)
{
}

string expr_special_form_t::to_string() {
  return "<#special form (?)>";
}

void expr_special_form_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_macro_t::expr_macro_t(const function<expr_t*(expr_t*)>& f):
  base(expr_type_t::MACRO),
  f(f)
{
}

string expr_macro_t::to_string() {
  return "macro";
}

void expr_macro_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_compound_proc_t::expr_compound_proc_t(expr_t* params, expr_t* body):
  base(expr_type_t::COMPOUND_PROC),
  params(params),
  body(body)
{
}

string expr_compound_proc_t::to_string() {
  return "compound proc";
}

void expr_compound_proc_t::print(ostream& os, const string& prefix, bool is_last) {
  params->print(os, prefix, false);
  body->print(os, prefix, true);
}

expr_cons_t::expr_cons_t(expr_t* first, expr_t* second):
  base(expr_type_t::CONS),
  first(first),
  second(second)
{
}

string expr_cons_t::to_string() {
  string result = "(";
  expr_t* cur = (expr_t*)this;
  while (cur->type == expr_type_t::CONS) {
    expr_cons_t* cons = (expr_cons_t*)cur;
    result += cons->first->to_string();
    
    if (cons->second->type == expr_type_t::CONS) {
      result += " ";
      cur = cons->second;
    } else if (cons->second->type == expr_type_t::NIL) {
      break ;
    } else {
      result += " . " + cons->second->to_string();
      break ;
    }
  }
  result += ")";

  return result;
}

void expr_cons_t::print(ostream& os, const string& prefix, bool is_last) {
  first->print(os, prefix, false);
  second->print(os, prefix, true);
}

expr_istream_t::expr_istream_t(istream& is):
  base(expr_type_t::ISTREAM),
  is(is)
{
}

string expr_istream_t::to_string() {
  return "istream";
}

void expr_istream_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_ostream_t::expr_ostream_t(ostream& os):
  base(expr_type_t::OSTREAM),
  os(os)
{
}

string expr_ostream_t::to_string() {
  return "ostream";
}

void expr_ostream_t::print(ostream& os, const string& prefix, bool is_last) {
}

expr_exception_t::expr_exception_t(const string& message, expr_t* expr):
  expr(expr),
  message(message)
{
}

const char* expr_exception_t::what() const noexcept {
  return message.c_str();
}

