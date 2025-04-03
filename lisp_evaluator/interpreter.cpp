#include "interpreter.h"

const char* expr_type_to_str(expr_type_t expr_type) {
  switch (expr_type) {
  case expr_type_t::NIL: return "NIL";
  case expr_type_t::INTEGER: return "INTEGER";
  case expr_type_t::REAL: return "REAL";
  case expr_type_t::STRING: return "STRING";
  case expr_type_t::SYMBOL: return "SYMBOL";
  case expr_type_t::ENV: return "ENV";
  case expr_type_t::PRIMITIVE_PROC: return "PRIMITIVE PROC";
  case expr_type_t::SPECIAL_FORM: return "SPECIAL FORM";
  case expr_type_t::COMPOUND_PROC: return "COMPOUND PROC";
  case expr_type_t::CONS: return "CONS";
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
  case expr_type_t::NIL: {
    return ((expr_nil_t*)this)->to_string();
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
  case expr_type_t::COMPOUND_PROC: {
    return ((expr_compound_proc_t*)this)->to_string();
  } break ;
  case expr_type_t::CONS: {
    return ((expr_cons_t*)this)->to_string();
  } break ;
  default: assert(0);
  }
}

void expr_t::print(ostream& os, const string& prefix, bool is_last) {
  os << prefix << (is_last ? "└── " : "├── ") << to_string() << endl;
  string new_prefix = prefix + (is_last ? "    " : "│   ");

  switch (type) {
  case expr_type_t::NIL: {
    ((expr_nil_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::INTEGER: {
    ((expr_integer_t*)this)->print(os, new_prefix, is_last);
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
  case expr_type_t::COMPOUND_PROC: {
    ((expr_compound_proc_t*)this)->print(os, new_prefix, is_last);
  } break ;
  case expr_type_t::CONS: {
    ((expr_cons_t*)this)->print(os, new_prefix, is_last);
  } break ;
  default: assert(0);
  }
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
  next(0)
{
}

expr_t* expr_env_t::lookup(expr_t* symbol) {
  return lookup_internal(symbol)->second;
}

expr_t* expr_env_t::set(expr_t* symbol, expr_t* expr) {
  if (symbol->type != expr_type_t::SYMBOL) {
    throw expr_exception_t("set: symbol expected", symbol);
  }
  auto it = lookup_internal(symbol);
  it->second = expr;
  return expr;
}

expr_t* expr_env_t::define(expr_t* symbol, expr_t* expr) {
  if (symbol->type != expr_type_t::SYMBOL) {
    throw expr_exception_t("define: symbol expected", symbol);
  }
  bindings[symbol] = expr;
  return expr;
}

map<expr_t*, expr_t*>::iterator expr_env_t::lookup_internal(expr_t* symbol) {
  auto it = bindings.find(symbol);
  if (it == bindings.end()) {
    if (!next) {
      throw expr_exception_t("lookup_internal: symbol is not defined", symbol);
    }
    return next->lookup_internal(symbol);
  }
  return it;
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

expr_exception_t::expr_exception_t(const string& message, expr_t* expr):
  expr(expr),
  message(message)
{
}


const char* expr_exception_t::what() const noexcept {
  return message.c_str();
}

interpreter_t::interpreter_t() {
  nil = (expr_t*) new expr_nil_t();
  t = make_symbol("#t");
  global_env.define(make_symbol("car"), make_primitive_proc([this](expr_t* expr) { return car(expr); }, 1, false));
  global_env.define(make_symbol("cdr"), make_primitive_proc([this](expr_t* expr) { return cdr(expr); }, 1, false));
  global_env.define(make_symbol("cons"), make_primitive_proc([this](expr_t* expr) { return make_cons(car(expr), cdr(expr)); }, 2, false));
  global_env.define(make_symbol("list"), make_primitive_proc([this](expr_t* expr) { return expr; }, 0, true));
  global_env.define(make_symbol("eq?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_eq(car(expr), car(cdr(expr)))); }, 2, false));
  global_env.define(make_symbol("symbol?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_symbol(expr)); }, 1, false));
  global_env.define(make_symbol("string?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_string(expr)); }, 1, false));
  global_env.define(make_symbol("list?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_list(expr)); }, 1, false));
  global_env.define(make_symbol("null?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_nil(expr)); }, 1, false));
  global_env.define(make_symbol("integer?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_integer(expr)); }, 1, false));
  global_env.define(make_symbol("pair?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_cons(expr)); }, 1, false));
  global_env.define(make_symbol("+"), make_primitive_proc([this](expr_t* expr) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_add(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("-"), make_primitive_proc([this](expr_t* expr) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_sub(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("*"), make_primitive_proc([this](expr_t* expr) {
    expr_t* result = make_real(1.0);
    while (!is_nil(expr)) {
      result = number_mul(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("/"), make_primitive_proc([this](expr_t* expr) {
    if (list_length_internal(expr) == 1) {
      return number_div(make_real(1.0), car(expr));
    }
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_div(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));


  global_env.define(make_symbol("quote"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    return expr;
  }));
  global_env.define(make_symbol("if"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    // (if condition consequence alternative)
    if (is_true(eval(list_ref(expr, 1), env))) {
      return eval(list_ref(expr, 2), env);
    } else if (!is_nil(list_tail(expr, 3))) {
      return eval(list_ref(expr, 3), env);
    } else {
      return make_nil();
    }
  }));
  global_env.define(make_symbol("define"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    if (is_list(car(expr))) {
      // (define (fn args ...) body)
      return env->define(car(car(expr)), make_compound_proc(cdr(car(expr)), car(cdr(expr))));
    } else if (is_symbol(car(expr))) {
      // (define var val)
      return env->define(car(expr), eval(list_ref(expr, 1), env));
    } else {
      throw expr_exception_t("unrecognized special form of define", expr);
    }
  }));
  //define_primitive_proc(make_primitive_proc([this](expr_t* expr) {
  //}, (expr_t*) new expr_symbol_t("when"), 1, false));
}

static token_t eat_token(lexer_t& lexer) {
  token_t token = lexer.eat_token();
  while (token.type == token_type_t::COMMENT) {
    token = lexer.eat_token();
  }
  if (token.type == token_type_t::ERROR) {
    throw token_exception_t("unexpected error token", token);
  }
  return token;
}

expr_t* interpreter_t::read(lexer_t& lexer) {
  return read(lexer, eat_token(lexer));
}

expr_t* interpreter_t::read(lexer_t& lexer, token_t token) {
  switch (token.type) {
  case token_type_t::NIL: return read_nil();
  case token_type_t::NUMBER: return read_number(token);
  case token_type_t::STRING: return read_string(token);
  case token_type_t::APOSTROPHE: return read_apostrophe(lexer, token);
  case token_type_t::LEFT_PAREN: return read_list(lexer);
  case token_type_t::ERROR: throw token_exception_t("unexpected token", token);
  case token_type_t::END_OF_FILE: return 0;
  default: return read_symbol(token);
  }
  return 0;
}

expr_t* interpreter_t::read_nil() {
  return make_nil();
}

expr_t* interpreter_t::read_number(token_t token) {
  double real = stod(token.to_string());
  if ((int64_t) real == real) {
    return make_integer((int64_t) real);
  } else {
    return make_real(real);
  }
}

expr_t* interpreter_t::read_string(token_t token) {
  return make_string(token.to_string());
}

expr_t* interpreter_t::read_symbol(token_t token) {
  return make_symbol(token.to_string());
}

expr_t* interpreter_t::read_list(lexer_t& lexer) {
  expr_t* first = 0;
  expr_t* prev = 0;
  token_t token = eat_token(lexer);
  while (token.type != token_type_t::END_OF_FILE && token.type != token_type_t::RIGHT_PAREN) {
    expr_t* expr = read(lexer, token);
    if (!expr) {
      throw token_exception_t("expect expression", token);
    }
    expr_t* cur = make_cons(expr, make_nil());
    if (!first) {
      first = cur;
    } else {
      set_cdr(prev, cur);
    }
    prev = cur;
    token = eat_token(lexer);
  }
  if (token.type != token_type_t::RIGHT_PAREN) {
    throw token_exception_t("expect right paren", token);
  }
  if (first) {
    return first;
  }
  return make_nil();
}

expr_t* interpreter_t::read_apostrophe(lexer_t& lexer, token_t token) {
  expr_t* quoted_expr = read(lexer, eat_token(lexer));
  if (!quoted_expr) {
    throw token_exception_t("expected expression", token);
  }
  return make_cons(make_symbol("quote"), quoted_expr);
}

expr_t* interpreter_t::eval(expr_t* expr) {
  return eval(expr, &global_env);
}

expr_t* interpreter_t::eval(expr_t* expr, expr_env_t* env) {
  switch (expr->type) {
  case expr_type_t::NIL:
  case expr_type_t::INTEGER:
  case expr_type_t::REAL:
  case expr_type_t::STRING: return expr;
  case expr_type_t::SYMBOL: return env->lookup(expr);
  case expr_type_t::CONS: return apply(eval(car(expr), env), cdr(expr), env);
  case expr_type_t::ENV:
  case expr_type_t::PRIMITIVE_PROC: 
  case expr_type_t::SPECIAL_FORM:
  case expr_type_t::COMPOUND_PROC: throw expr_exception_t("unexpected type in eval", expr);
  default: assert(0);
  }
}

expr_t* interpreter_t::apply(expr_t* expr, expr_t* args, expr_env_t* env) {
  if (is_special_form(expr)) {
    return apply_special_form(expr, args, env);
  } else if (is_primitive_proc(expr)) {
    return apply_primitive_proc(expr, args, env);
  } else if (is_compound_proc(expr)) {
    return apply_compound_proc(expr, args, env);
  } else {
    throw expr_exception_t("unexpected type in apply", expr);
  }
}

expr_t* interpreter_t::apply_special_form(expr_t* expr, expr_t* args, expr_env_t* env) {
  assert(expr->type == expr_type_t::SPECIAL_FORM);
  return ((expr_special_form_t*)expr)->f(args, env);
}

expr_t* interpreter_t::apply_primitive_proc(expr_t* expr, expr_t* args, expr_env_t* env) {
  int len = list_length_internal(args);
  if (len == -1) {
    throw expr_exception_t("argument list has cycles for primitive procedure", args);
  }
  expr_primitive_proc_t* proc = (expr_primitive_proc_t*)expr;
  if (proc->is_variadic && len < proc->arity || !proc->is_variadic && len != proc->arity) {
    throw expr_exception_t("argument list arity is not defined for primitive procedure", args);
  }
  return proc->f(list_map(args, [this, env](expr_t* expr) {
    return eval(expr, env);
  }));
}

expr_t* interpreter_t::apply_compound_proc(expr_t* expr, expr_t* args, expr_env_t* env) {
  expr_t* body = get_compound_proc_body(expr);
  expr_t* params = get_compound_proc_params(expr);
  expr_env_t extended_env = extend_env(env, list_map(args, [this, env](expr_t* expr) {
    return eval(expr, env);
  }), params);
  expr_t* last = make_nil();
  while (!is_nil(body)) {
    last = eval(car(body), &extended_env);
    body = cdr(body);
  }
  return last;
}

expr_t* interpreter_t::make_nil() {
  return nil;
}

bool interpreter_t::is_nil(expr_t* expr) {
  return expr->type == expr_type_t::NIL;
}

expr_t* interpreter_t::make_boolean(bool boolean) {
  return boolean ? t : make_nil();
}

bool interpreter_t::is_true(expr_t* expr) {
  return !is_false(expr);
}

bool interpreter_t::is_false(expr_t* expr) {
  return is_nil(expr);
}

expr_t* interpreter_t::make_integer(int64_t integer) {
  return (expr_t*) new expr_integer_t(integer);
}

bool interpreter_t::is_integer(expr_t* expr) {
  return expr->type == expr_type_t::INTEGER;
}

int64_t interpreter_t::get_integer(expr_t* expr) {
  if (!is_integer(expr)) {
    throw expr_exception_t("unexpected type in get integer", expr);
  }
  return ((expr_integer_t*)expr)->integer;
}

expr_t* interpreter_t::make_real(double real) {
  return (expr_t*) new expr_real_t(real);
}

bool interpreter_t::is_real(expr_t* expr) {
  return expr->type == expr_type_t::REAL;
}

double interpreter_t::get_real(expr_t* expr) {
  if (is_integer(expr)) {
    return (double) get_integer(expr);
  } else if (is_real(expr)) {
    return ((expr_real_t*)expr)->real;
  } else {
    throw expr_exception_t("unexpected type in get real", expr);
  }
}

expr_t* interpreter_t::number_add(expr_t* a, expr_t* b) {
  double result = get_real(a) + get_real(b);
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_sub(expr_t* a, expr_t* b) {
  double result = get_real(a) - get_real(b);
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_mul(expr_t* a, expr_t* b) {
  double result = get_real(a) * get_real(b);
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_div(expr_t* a, expr_t* b) {
  double denom = get_real(b);
  if (denom == 0.0) {
    throw expr_exception_t("cannot divide by 0", b);
  }
  double result = get_real(a) / denom;
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::make_string(const string& str) {
  auto it = interned_strings.find(str);
  if (it != interned_strings.end()) {
    return it->second;
  }
  expr_t* result = (expr_t*) new expr_string_t(str);
  interned_strings[str] = result;
  return result;
}

bool interpreter_t::is_string(expr_t* expr) {
  return expr->type == expr_type_t::STRING;
}

string interpreter_t::get_string(expr_t* expr) {
  if (!is_string(expr)) {
    throw expr_exception_t("unexpected type in get string", expr);
  }
  return ((expr_string_t*)expr)->str;
}

expr_t* interpreter_t::make_symbol(const string& symbol) {
  auto it = interned_strings.find(symbol);
  if (it != interned_strings.end()) {
    return it->second;
  }
  expr_t* result = (expr_t*) new expr_symbol_t(symbol);
  interned_strings[symbol] = result;
  return result;
}

bool interpreter_t::is_symbol(expr_t* expr) {
  return expr->type == expr_type_t::SYMBOL;
}

string interpreter_t::get_symbol(expr_t* expr) {
  if (!is_symbol(expr)) {
    throw expr_exception_t("unexpected type in get symbol", expr);
  }
  return ((expr_symbol_t*)expr)->symbol;
}

expr_t* interpreter_t::make_env() {
  return (expr_t*) new expr_env_t();
}

expr_env_t interpreter_t::extend_env(expr_env_t* env, expr_t* symbols, expr_t* exprs) {
  expr_env_t result;
  result.next = env;
  int len_symbols = list_length_internal(symbols);
  int len_exprs = list_length_internal(exprs);
  if (len_symbols == -1) {
    throw expr_exception_t("symbols list has cycle", symbols);
  } else if (len_exprs == -1) {
    throw expr_exception_t("exprs list has cycle", exprs);
  } else if (len_symbols < len_exprs) {
    throw expr_exception_t("symbols list is smaller than exprs list", symbols);
  } else if (len_exprs < len_symbols) {
    throw expr_exception_t("exprs list is smaller than symbols list", exprs);
  }

  for (int i = 0; i < len_symbols; ++i) {
    result.define(car(symbols), car(exprs));
    symbols = cdr(symbols);
    exprs = cdr(exprs);
  }
  return result;
}

expr_t* interpreter_t::make_primitive_proc(const function<expr_t*(expr_t*)>& f, int arity, bool is_variadic) {
  return (expr_t*) new expr_primitive_proc_t(f, arity, is_variadic);
}

bool interpreter_t::is_primitive_proc(expr_t* expr) {
  return expr->type == expr_type_t::PRIMITIVE_PROC;
}

expr_t* interpreter_t::make_special_form(const function<expr_t*(expr_t*, expr_env_t*)>& f) {
  return (expr_t*) new expr_special_form_t(f);
}

bool interpreter_t::is_special_form(expr_t* expr) {
  return expr->type == expr_type_t::SPECIAL_FORM;
}

expr_t* interpreter_t::make_compound_proc(expr_t* params, expr_t* body) {
  return (expr_t*) new expr_compound_proc_t(params, body);
}

bool interpreter_t::is_compound_proc(expr_t* expr) {
  return expr->type == expr_type_t::COMPOUND_PROC;
}

expr_t* interpreter_t::get_compound_proc_params(expr_t* expr) {
  if (!is_compound_proc(expr)) {
    throw expr_exception_t("get_compound_proc_params: compound proc expected", expr);
  }
  return ((expr_compound_proc_t*)expr)->params;
}

expr_t* interpreter_t::get_compound_proc_body(expr_t* expr) {
  if (!is_compound_proc(expr)) {
    throw expr_exception_t("get_compound_proc_body: compound proc expected", expr);
  }
  return ((expr_compound_proc_t*)expr)->body;
}

expr_t* interpreter_t::make_cons(expr_t* expr1, expr_t* expr2) {
  return (expr_t*) new expr_cons_t(expr1, expr2);
}

expr_t* interpreter_t::make_list(const initializer_list<expr_t*>& exprs) {
  expr_t* result = make_nil();
  for (auto cur = rbegin(exprs); cur != rend(exprs); ++cur) {
    result = make_cons(*cur, result);
  }
  return result;
}

bool interpreter_t::is_cons(expr_t* expr) {
  return expr->type == expr_type_t::CONS;
}

expr_t* interpreter_t::car(expr_t* expr) {
  if (!is_cons(expr)) {
    throw expr_exception_t("car: cons expected", expr);
  }
  return ((expr_cons_t*)expr)->first;
}

expr_t* interpreter_t::cdr(expr_t* expr) {
  if (!is_cons(expr)) {
    throw expr_exception_t("cdr: cons expected", expr);
  }
  return ((expr_cons_t*)expr)->second;
}

void interpreter_t::set_car(expr_t* expr, expr_t* val) {
  if (!is_cons(expr)) {
    throw expr_exception_t("set_car: cons expected", expr);
  }
  ((expr_cons_t*)expr)->first = val;
}

void interpreter_t::set_cdr(expr_t* expr, expr_t* val) {
  if (!is_cons(expr)) {
    throw expr_exception_t("set_cdr: cons expected", expr);
  }
  ((expr_cons_t*)expr)->second = val;
}

bool interpreter_t::is_eq(expr_t* expr1, expr_t* expr2) {
  return expr1 == expr2;
}

bool interpreter_t::is_list(expr_t* expr) {
  while (is_cons(expr)) {
    expr = cdr(expr);
  }
  return is_nil(expr);
}

expr_t* interpreter_t::list_tail(expr_t* expr, size_t n) {
  while (n--) {
    expr = cdr(expr);
  }
  return expr;
}

expr_t* interpreter_t::list_tail(expr_t* expr, expr_t* integer_expr) {
  if (!is_integer(integer_expr)) {
    throw expr_exception_t("list_tail: integer expected", integer_expr);
  }
  return list_tail(expr, get_integer(integer_expr));
}

expr_t* interpreter_t::list_ref(expr_t* expr, size_t n) {
  return car(list_tail(expr, n));
}

expr_t* interpreter_t::list_ref(expr_t* expr, expr_t* integer_expr) {
  return car(list_tail(expr, integer_expr));
}

int interpreter_t::list_length_internal(expr_t* expr) {
  int result = 0;
  expr_t* turtle = expr;
  expr_t* hare = expr;
  while (!is_nil(hare)) {
    turtle = cdr(turtle);
    hare = cdr(hare);
    ++result;
    if (!is_nil(hare)) {
      hare = cdr(hare);
      ++result;
    } else {
      break ;
    }
    if (hare == turtle) {
      return -1;
    }
  }
  return result;
}

expr_t* interpreter_t::list_length(expr_t* expr) {
  int result = list_length_internal(expr);
  if (result == -1) {
    return make_nil();
  }
  return make_integer(result);
}

expr_t* interpreter_t::list_map(expr_t* expr, const function<expr_t*(expr_t*)>& f) {
  if (is_nil(expr)) {
    return make_nil();
  }
  return make_cons(f(car(expr)), list_map(cdr(expr), f));
}

bool interpreter_t::is_tagged(expr_t* expr, expr_t* tag) {
  return is_cons(expr) && is_eq(car(expr), tag);
}

bool interpreter_t::is_tagged(expr_t* expr, const string& symbol) {
  return is_cons(expr) && is_symbol(car(expr)) && get_symbol(car(expr)) == symbol;
}


