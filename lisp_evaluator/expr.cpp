#include "expr.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::END_OF_FILE: return "EOF";
  case expr_type_t::VOID: return "VOID";
  case expr_type_t::NIL: return "NIL";
  case expr_type_t::BOOLEAN: return "BOOLEAN";
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

expr_eof_t::expr_eof_t():
  base(expr_type_t::END_OF_FILE)
{
}

expr_void_t::expr_void_t():
  base(expr_type_t::VOID)
{
}

expr_nil_t::expr_nil_t():
  base(expr_type_t::NIL)
{
}

expr_boolean_t::expr_boolean_t(bool boolean):
  base(expr_type_t::BOOLEAN),
  boolean(boolean)
{
}

expr_char_t::expr_char_t(char c):
  base(expr_type_t::CHAR),
  c(c)
{
}

expr_integer_t::expr_integer_t(int64_t integer):
  base(expr_type_t::INTEGER),
  integer(integer)
{
}

expr_real_t::expr_real_t(double real):
  base(expr_type_t::REAL),
  real(real)
{
}

expr_string_t::expr_string_t(const string& str):
  base(expr_type_t::STRING),
  str(str)
{
}

expr_symbol_t::expr_symbol_t(string symbol):
  base(expr_type_t::SYMBOL),
  symbol(symbol)
{
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

expr_primitive_proc_t::expr_primitive_proc_t(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f):
  base(expr_type_t::PRIMITIVE_PROC),
  name(name),
  f(f)
{
}

expr_special_form_t::expr_special_form_t(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f):
  base(expr_type_t::SPECIAL_FORM),
  name(name),
  f(f)
{
}

expr_macro_t::expr_macro_t(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f):
  base(expr_type_t::MACRO),
  name(name),
  f(f)
{
}

expr_compound_proc_t::expr_compound_proc_t(expr_t* params, expr_t* body):
  base(expr_type_t::COMPOUND_PROC),
  params(params),
  body(body)
{
}

expr_cons_t::expr_cons_t(expr_t* first, expr_t* second):
  base(expr_type_t::CONS),
  first(first),
  second(second)
{
}

expr_istream_t::expr_istream_t(istream& is):
  base(expr_type_t::ISTREAM),
  is(&is, [](istream*) {})
{
}

expr_istream_t::expr_istream_t(unique_ptr<istream> is):
  base(expr_type_t::ISTREAM),
  is(is.release(), [](istream* is) { delete is; })
{
}

expr_ostream_t::expr_ostream_t(ostream& os):
  base(expr_type_t::OSTREAM),
  os(&os, [](ostream*) {})
{
}

expr_ostream_t::expr_ostream_t(unique_ptr<ostream> os):
  base(expr_type_t::OSTREAM),
  os(os.release(), [](ostream* os) { delete os; })
{
}

expr_exception_t::expr_exception_t(const string& message, expr_t* expr):
  expr(expr),
  message(message)
{
}

const char* expr_exception_t::what() const noexcept {
  return message.c_str();
}

bool is_eof(expr_t* expr) {
  return expr->type == expr_type_t::END_OF_FILE;
}

bool is_void(expr_t* expr) {
  return expr->type == expr_type_t::VOID;
}

bool is_nil(expr_t* expr) {
  return expr->type == expr_type_t::NIL;
}

bool is_boolean(expr_t* expr) {
  return expr->type == expr_type_t::BOOLEAN;
}

bool get_boolean(expr_t* expr) {
  return ((expr_boolean_t*)expr)->boolean;
}

bool is_char(expr_t* expr) {
  return expr->type == expr_type_t::CHAR;
}

char get_char(expr_t* expr) {
  if (!is_char(expr)) {
    throw expr_exception_t("expect char", expr);
  }
  return ((expr_char_t*)expr)->c;
}

bool is_cons(expr_t* expr) {
  return expr->type == expr_type_t::CONS;
}

expr_t* car(expr_t* expr) {
  if (!is_cons(expr)) {
    throw expr_exception_t("car: cons expected", expr);
  }
  return ((expr_cons_t*)expr)->first;
}

expr_t* cdr(expr_t* expr) {
  if (!is_cons(expr)) {
    throw expr_exception_t("cdr: cons expected", expr);
  }
  return ((expr_cons_t*)expr)->second;
}

void set_car(expr_t* expr, expr_t* val) {
  if (!is_cons(expr)) {
    throw expr_exception_t("set_car: cons expected", expr);
  }
  ((expr_cons_t*)expr)->first = val;
}

void set_cdr(expr_t* expr, expr_t* val) {
  if (!is_cons(expr)) {
    throw expr_exception_t("set_cdr: cons expected", expr);
  }
  ((expr_cons_t*)expr)->second = val;
}

bool is_integer(expr_t* expr) {
  return expr->type == expr_type_t::INTEGER;
}

int64_t get_integer(expr_t* expr) {
  if (!is_integer(expr)) {
    throw expr_exception_t("get_integer: expect integer", expr);
  }
  return ((expr_integer_t*)expr)->integer;
}

bool is_real(expr_t* expr) {
  return expr->type == expr_type_t::REAL;
}

double get_real(expr_t* expr) {
  if (is_integer(expr)) {
    return (double) get_integer(expr);
  } else if (is_real(expr)) {
    return ((expr_real_t*)expr)->real;
  } else {
    throw expr_exception_t("unexpected type in get real", expr);
  }
}

bool is_string(expr_t* expr) {
  return expr->type == expr_type_t::STRING;
}

string get_string(expr_t* expr) {
  if (!is_string(expr)) {
    throw expr_exception_t("unexpected type in get string", expr);
  }
  return ((expr_string_t*)expr)->str;
}

bool is_symbol(expr_t* expr) {
  return expr->type == expr_type_t::SYMBOL;
}

string get_symbol(expr_t* expr) {
  if (!is_symbol(expr)) {
    throw expr_exception_t("unexpected type in get symbol", expr);
  }
  return ((expr_symbol_t*)expr)->symbol;
}

bool is_env(expr_t* expr) {
  return expr->type == expr_type_t::ENV;
}

unordered_map<expr_t*, expr_t*>& get_env_bindings(expr_t* expr) {
  return ((expr_env_t*)expr)->bindings;
}

bool is_primitive_proc(expr_t* expr) {
  return expr->type == expr_type_t::PRIMITIVE_PROC;
}

string get_primitive_proc_name(expr_t* expr) {
  if (!is_primitive_proc(expr)) {
    throw expr_exception_t("get_primitive_proc_name: primitive_proc expected", expr);
  }
  return ((expr_primitive_proc_t*)expr)->name;
}

bool is_macro(expr_t* expr) {
  return expr->type == expr_type_t::MACRO;
}

string get_macro_name(expr_t* expr) {
  if (!is_macro(expr)) {
    throw expr_exception_t("get_macro_name: macro expected", expr);
  }
  return ((expr_macro_t*)expr)->name;
}

bool is_special_form(expr_t* expr) {
  return expr->type == expr_type_t::SPECIAL_FORM;
}

string get_special_form_name(expr_t* expr) {
  if (!is_special_form(expr)) {
    throw expr_exception_t("get_special_form: special form expected", expr);
  }
  return ((expr_special_form_t*)expr)->name;
}

bool is_compound_proc(expr_t* expr) {
  return expr->type == expr_type_t::COMPOUND_PROC;
}

expr_t* get_compound_proc_params(expr_t* expr) {
  if (!is_compound_proc(expr)) {
    throw expr_exception_t("get_compound_proc_params: compound proc expected", expr);
  }
  return ((expr_compound_proc_t*)expr)->params;
}

expr_t* get_compound_proc_body(expr_t* expr) {
  if (!is_compound_proc(expr)) {
    throw expr_exception_t("get_compound_proc_body: compound proc expected", expr);
  }
  return ((expr_compound_proc_t*)expr)->body;
}

bool is_istream(expr_t* expr) {
  return expr->type == expr_type_t::ISTREAM;
}

istream& get_istream(expr_t* expr) {
  if (!is_istream(expr)) {
    throw expr_exception_t("get_istream: expect istream", expr);
  }
  return *((expr_istream_t*)expr)->is;
}

bool is_ostream(expr_t* expr) {
  return expr->type == expr_type_t::OSTREAM;
}

ostream& get_ostream(expr_t* expr) {
  if (!is_ostream(expr)) {
    throw expr_exception_t("get_ostream: expect ostream", expr);
  }
  return *((expr_ostream_t*)expr)->os;
}

// ---

string to_string(expr_t* expr) {
  switch (expr->type) {
  case expr_type_t::END_OF_FILE: {
    return "EOF";
  } break ;
  case expr_type_t::VOID: {
    assert("void expression must be compiled out" && 0);
    return "";
  } break ;
  case expr_type_t::NIL: {
    return "()";
  } break ;
  case expr_type_t::BOOLEAN: {
    return get_boolean(expr) ? "true" : "false";
  } break ;
  case expr_type_t::CHAR: {
    // need the mapping from char to symbol
    return "'" + string(1, get_char(expr)) + "'";
  } break ;
  case expr_type_t::INTEGER: {
    return std::to_string(get_integer(expr));
  } break ;
  case expr_type_t::REAL: {
    return std::to_string(get_real(expr));
  } break ;
  case expr_type_t::STRING: {
    return "\"" + get_string(expr) + "\"";
  } break ;
  case expr_type_t::SYMBOL: {
    return get_symbol(expr);
  } break ;
  case expr_type_t::ENV: {
    string result = "env: ";
    for (const auto& p : get_env_bindings(expr)) {
      result += "{ " + to_string(p.first) + ": " + (p.second == (expr_t*) expr ? "expr" : to_string(p.second)) + " } ";
    }
    return result;
  } break ;
  case expr_type_t::PRIMITIVE_PROC: {
    return "#<primitive procedure '" + get_primitive_proc_name(expr) + "'>";
  } break ;
  case expr_type_t::SPECIAL_FORM: {
    return "#<special form '" + get_special_form_name(expr) + "'>";
  } break ;
  case expr_type_t::MACRO: {
    return "#<macro '" + get_macro_name(expr) + "'>";
  } break ;
  case expr_type_t::COMPOUND_PROC: {
    return "#<compound proc: params: { " + to_string(get_compound_proc_params(expr)) + " }, body: { " + to_string(get_compound_proc_body(expr)) + " }>";
  } break ;
  case expr_type_t::CONS: {
    string result = "(";
    while (is_cons(expr)) {
      result += to_string(car(expr));
      if (is_cons(cdr(expr))) {
        result += " ";
        expr = cdr(expr);
      } else if (is_nil(cdr(expr))) {
        break ;
      } else {
        result += " . " + to_string(cdr(expr));
        break ;
      }
    }
    result += ")";
    return result;
  } break ;
  case expr_type_t::ISTREAM: {
    return "istream";
  } break ;
  case expr_type_t::OSTREAM: {
    return "ostream";
  } break ;
  default: assert(0);
  }
}

void print(expr_t* expr, ostream& os, const string& prefix, bool is_last) {
  os << prefix << (is_last ? "└── " : "├── ") << to_string(expr) << endl;
  string new_prefix = prefix + (is_last ? "    " : "│   ");

  switch (expr->type) {
  case expr_type_t::END_OF_FILE: {
  } break ;
  case expr_type_t::VOID: {
    assert("void expression must be compiled out" && 0);
  } break ;
  case expr_type_t::NIL: {
  } break ;
  case expr_type_t::BOOLEAN: {
  } break ;
  case expr_type_t::INTEGER: {
  } break ;
  case expr_type_t::CHAR: {
  } break ;
  case expr_type_t::REAL: {
  } break ;
  case expr_type_t::STRING: {
  } break ;
  case expr_type_t::SYMBOL: {
  } break ;
  case expr_type_t::ENV: {
  } break ;
  case expr_type_t::PRIMITIVE_PROC: {
  } break ;
  case expr_type_t::SPECIAL_FORM: {
  } break ;
  case expr_type_t::MACRO: {
  } break ;
  case expr_type_t::COMPOUND_PROC: {
    print(get_compound_proc_params(expr), os, new_prefix, false);
    print(get_compound_proc_body(expr), os, new_prefix, true);
  } break ;
  case expr_type_t::CONS: {
    print(car(expr), os, prefix, false);
    print(cdr(expr), os, prefix, true);
  } break ;
  case expr_type_t::ISTREAM: {
  } break ;
  case expr_type_t::OSTREAM: {
  } break ;
  default: assert(0);
  }
}

void print(expr_t* expr, ostream& os) {
  print(expr, os, "", true);
}

bool is_list(expr_t* expr) {
  while (is_cons(expr)) {
    expr = cdr(expr);
  }
  return is_nil(expr);
}

