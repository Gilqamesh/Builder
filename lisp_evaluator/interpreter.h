#ifndef PARSER_H
# define PARSER_H

# include "lexer.h"

enum class expr_type_t : int {
  NIL,
  INTEGER,
  REAL,
  STRING,
  SYMBOL,
  ENV,
  PRIMITIVE_PROC,
  SPECIAL_FORM,
  COMPOUND_PROC,
  CONS
};

const char* expr_type_to_str(expr_type_t expr_type);

struct expr_t {
  expr_t(expr_type_t type);

  expr_type_t type;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_nil_t {
  expr_nil_t();

  expr_t base;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_integer_t {
  expr_integer_t(int64_t integer);

  expr_t base;
  int64_t integer;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_real_t {
  expr_real_t(double real);

  expr_t base;
  double real;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_string_t {
  expr_string_t(const string& str);

  expr_t base;
  string str;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_symbol_t {
  expr_symbol_t(string symbol);

  expr_t base;
  string symbol;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_env_t {
  expr_env_t();

  expr_t base;
  map<expr_t*, expr_t*> bindings;
  expr_env_t* next;

  expr_t* lookup(expr_t* symbol);
  expr_t* set(expr_t* symbol, expr_t* expr);
  expr_t* define(expr_t* symbol, expr_t* expr);

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);

private:
  map<expr_t*, expr_t*>::iterator lookup_internal(expr_t* symbol);
};

struct expr_primitive_proc_t {
  expr_primitive_proc_t(const function<expr_t*(expr_t*)>& f, int arity, bool is_variadic);

  expr_t base;
  function<expr_t*(expr_t*)> f;
  int arity;
  bool is_variadic;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_special_form_t {
  expr_special_form_t(const function<expr_t*(expr_t*, expr_env_t*)>& f);

  expr_t base;
  function<expr_t*(expr_t*, expr_env_t*)> f;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_compound_proc_t {
  expr_compound_proc_t(expr_t* params, expr_t* body);

  expr_t base;
  expr_t* params;
  expr_t* body;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_cons_t {
  expr_cons_t(expr_t* first, expr_t* second);

  expr_t base;
  expr_t* first;
  expr_t* second;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_exception_t : public exception {
  expr_exception_t(const string& message, expr_t* expr);

  expr_t* expr;
  string message;

  const char* what() const noexcept override;
};

struct interpreter_t {
  interpreter_t();

  expr_t* read(istream& is);
  expr_t* eval(expr_t* expr);
  void print(ostream& os, expr_t* expr);

private:
  // todo: add system_env where all initial stuff lives, expose user_env to users
  expr_env_t global_env;
  expr_t* nil;
  expr_t* t;
  unordered_map<string, expr_t*> interned_strings;

  expr_t* define_primitive_proc(expr_t* expr);
  expr_t* define_special_form(expr_t* expr);

  expr_t* read(lexer_t& lexer, token_t token);
  expr_t* read_nil();
  expr_t* read_number(token_t token);
  expr_t* read_string(token_t token);
  expr_t* read_symbol(token_t token);
  expr_t* read_apostrophe(lexer_t& lexer, token_t token);
  expr_t* read_list(lexer_t& lexer);

  expr_t* eval(expr_t* expr, expr_env_t* env);

  expr_t* apply(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_primitive_proc(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_special_form(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_compound_proc(expr_t* expr, expr_t* args, expr_env_t* env);

  // ---

  expr_t* make_nil();
  bool is_nil(expr_t* expr);

  expr_t* make_boolean(bool boolean);
  bool is_true(expr_t* expr);
  bool is_false(expr_t* expr);

  expr_t* make_integer(int64_t integer);
  bool is_integer(expr_t* expr);
  int64_t get_integer(expr_t* expr);

  expr_t* make_real(double real);
  bool is_real(expr_t* expr);
  double get_real(expr_t* expr);

  expr_t* number_add(expr_t* a, expr_t* b);
  expr_t* number_sub(expr_t* a, expr_t* b);
  expr_t* number_mul(expr_t* a, expr_t* b);
  expr_t* number_div(expr_t* a, expr_t* b);
  expr_t* number_eq(expr_t* a, expr_t* b);

  expr_t* make_string(const string& str);
  bool is_string(expr_t* expr);
  string get_string(expr_t* expr);

  expr_t* make_symbol(const string& symbol);
  bool is_symbol(expr_t* expr);
  string get_symbol(expr_t* expr);

  expr_t* make_env();
  expr_env_t extend_env(expr_env_t* env, expr_t* symbols, expr_t* exprs);

  expr_t* make_primitive_proc(const function<expr_t*(expr_t*)>& f, int arity, bool is_variadic);
  bool is_primitive_proc(expr_t* expr);

  expr_t* make_special_form(const function<expr_t*(expr_t*, expr_env_t*)>& f);
  bool is_special_form(expr_t* expr);

  expr_t* make_compound_proc(expr_t* params, expr_t* body);
  bool is_compound_proc(expr_t* expr);
  expr_t* get_compound_proc_params(expr_t* expr);
  expr_t* get_compound_proc_body(expr_t* expr);

  expr_t* make_cons(expr_t* expr1, expr_t* expr2);
  expr_t* make_list(const initializer_list<expr_t*>& exprs);
  bool is_cons(expr_t* expr);
  expr_t* car(expr_t* expr);
  expr_t* cdr(expr_t* expr);
  void set_car(expr_t* expr, expr_t* val);
  void set_cdr(expr_t* expr, expr_t* val);

  // ---

  bool is_eq(expr_t* expr1, expr_t* expr2);

  bool is_list(expr_t* expr);
  expr_t* list_tail(expr_t* expr, int n);
  expr_t* list_tail(expr_t* expr, expr_t* integer_expr);
  expr_t* list_ref(expr_t* expr, int n);
  expr_t* list_ref(expr_t* expr, expr_t* integer_expr);
  int list_length_internal(expr_t* expr);
  expr_t* list_length(expr_t* expr);
  expr_t* list_map(expr_t* expr, const function<expr_t*(expr_t*)>& f);

  bool is_tagged(expr_t* expr, expr_t* symbol);
  bool is_tagged(expr_t* expr, const string& symbol);
};

#endif // PARSER_H

