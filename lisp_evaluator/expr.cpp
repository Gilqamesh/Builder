#include "expr.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::END_OF_FILE: return "EOF";
  case expr_type_t::NIL: return "NIL";
  case expr_type_t::VOID: return "VOID";
  case expr_type_t::BOOLEAN: return "BOOLEAN";
  case expr_type_t::CHAR: return "CHAR";
  case expr_type_t::INTEGER: return "INTEGER";
  case expr_type_t::REAL: return "REAL";
  case expr_type_t::STRING: return "STRING";
  case expr_type_t::SYMBOL: return "SYMBOL";
  case expr_type_t::ENV: return "ENV";
  case expr_type_t::MACRO: return "MACRO";
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

expr_nil_t::expr_nil_t():
  base(expr_type_t::NIL)
{
}

expr_void_t::expr_void_t():
  base(expr_type_t::VOID)
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
    cur = (expr_env_t*) cur->parent;
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
    cur = (expr_env_t*) cur->parent;
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

expr_macro_t::expr_macro_t(function<expr_t*(expr_t* args, expr_t* call_env)> f):
  base(expr_type_t::MACRO),
  f(f)
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

bool is_nil(expr_t* expr) {
  return expr->type == expr_type_t::NIL;
}

bool is_void(expr_t* expr) {
  return expr->type == expr_type_t::VOID;
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

expr_t* env_get_parent(expr_t* env) {
  if (!is_env(env)) {
    throw expr_exception_t("env_get_parent: env expected", env);
  }
  expr_env_t* expr_env = (expr_env_t*)env;
  return expr_env->parent;
}

void env_set_parent(expr_t* env, expr_t* new_parent) {
  if (!is_env(env)) {
    throw expr_exception_t("env_set_parent: env expected", env);
  }
  if (!is_env(new_parent)) {
    throw expr_exception_t("env_set_parent: env expected", new_parent);
  }
  expr_env_t* expr_env = (expr_env_t*)env;
  expr_env->parent = new_parent;
}

expr_t* env_define(expr_t* env, expr_t* symbol, expr_t* expr) {
  if (!is_env(env)) {
    throw expr_exception_t("env_define: env expected", env);
  }
  expr_env_t* expr_env = (expr_env_t*)env;
  return expr_env->define(symbol, expr);
}

expr_t* env_get(expr_t* env, expr_t* symbol) {
  if (!is_env(env)) {
    throw expr_exception_t("env_get: env expected", env);
  }
  expr_env_t* expr_env = (expr_env_t*)env;
  return expr_env->get(symbol);
}

expr_t* env_set(expr_t* env, expr_t* symbol, expr_t* expr) {
  if (!is_env(env)) {
    throw expr_exception_t("env_set: env expected", env);
  }
  expr_env_t* expr_env = (expr_env_t*)env;
  return expr_env->set(symbol, expr);
}

unordered_map<expr_t*, expr_t*>& get_env_bindings(expr_t* env) {
  if (!is_env(env)) {
    throw expr_exception_t("get_env_bindings: env expected", env);
  }
  return ((expr_env_t*)env)->bindings;
}

bool is_macro(expr_t* expr) {
  return expr->type == expr_type_t::MACRO;
}

const function<expr_t*(expr_t* args, expr_t* call_env)>& macro_f(expr_t* expr) {
  if (!is_macro(expr)) {
    throw expr_exception_t("macro_f: macro expected", expr);
  }
  return ((expr_macro_t*)expr)->f;
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

static string to_string_recursive(expr_t* const expr, unordered_set<expr_t*>& seen) {
  string result;

  if (seen.find(expr) != seen.end()) {
    return "...";
  }
  seen.insert(expr);

  switch (expr->type) {
  case expr_type_t::END_OF_FILE: {
    result = "EOF";
  } break ;
  case expr_type_t::NIL: {
    result = "()";
  } break ;
  case expr_type_t::VOID: {
    assert(0 && "to_string_recursive: void exprs should not be evaluated");
  } break ;
  case expr_type_t::BOOLEAN: {
    result = get_boolean(expr) ? "true" : "false";
  } break ;
  case expr_type_t::CHAR: {
    // need the mapping from char to symbol
    result = "'" + string(1, get_char(expr)) + "'";
  } break ;
  case expr_type_t::INTEGER: {
    result = std::to_string(get_integer(expr));
  } break ;
  case expr_type_t::REAL: {
    result = std::to_string(get_real(expr));
  } break ;
  case expr_type_t::STRING: {
    result = "\"" + get_string(expr) + "\"";
  } break ;
  case expr_type_t::SYMBOL: {
    result = get_symbol(expr);
  } break ;
  case expr_type_t::ENV: {
    size_t depth = 0;
    expr_t* parent = env_get_parent(expr);
    while (parent) {
      ++depth;
      parent = env_get_parent(parent);
    }
    result = "('env (depth " + to_string(depth) + ") ";
    auto& bindings = get_env_bindings(expr);
    result += "(";
    auto it = bindings.begin();
    while (it != bindings.end()) {
      auto nit = it;
      ++nit;
      result += "(" + to_string_recursive(it->first, seen) + " " + to_string_recursive(it->second, seen) + ")";
      it = nit;
      if (nit != bindings.end()) {
        result += " ";
      }
    }
    result += "))";
    if (expr_t* parent = env_get_parent(expr)) {
      result += " -> " + to_string_recursive(parent, seen);
    }
  } break ;
  case expr_type_t::MACRO: {
    result = "#<macro>";
  } break ;
  case expr_type_t::CONS: {
    result = "(";
    expr_t* cur = expr;
    while (is_cons(cur)) {
      result += to_string_recursive(car(cur), seen);
      if (is_cons(cdr(cur))) {
        result += " ";
        cur = cdr(cur);
      } else if (is_nil(cdr(cur))) {
        break ;
      } else {
        result += " . " + to_string_recursive(cdr(cur), seen);
        break ;
      }
    }
    result += ")";
  } break ;
  case expr_type_t::ISTREAM: {
    result = "istream";
  } break ;
  case expr_type_t::OSTREAM: {
    result = "ostream";
  } break ;
  default: assert(0);
  }

  seen.erase(expr);
  return result;
}

string to_string(expr_t* expr) {
  unordered_set<expr_t*> seen;
  return to_string_recursive(expr, seen);
}

void print(expr_t* expr, ostream& os, const string& prefix, bool is_last) {
  os << prefix << (is_last ? "└── " : "├── ") << to_string(expr) << endl;
  string new_prefix = prefix + (is_last ? "    " : "│   ");

  switch (expr->type) {
  case expr_type_t::END_OF_FILE: {
  } break ;
  case expr_type_t::NIL: {
  } break ;
  case expr_type_t::VOID: {
    assert(0 && "print: void exprs should not be evaluated");
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
  case expr_type_t::MACRO: {
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

