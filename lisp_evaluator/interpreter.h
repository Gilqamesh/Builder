#ifndef PARSER_H
# define PARSER_H

# include "lexer.h"
# include "expr.h"
# include "reader.h"

struct interpreter_t {
  interpreter_t();

  expr_t* read(istream& is);
  expr_t* eval(expr_t* expr);
  void print(ostream& os, expr_t* expr);

  void source(const char* filepath);
  void source(istream& is);
  void source(ostream& os, istream& is);

private:
  reader_t reader;

  // todo: add system_env where all initial stuff lives, expose user_env to users
  expr_env_t global_env;
  expr_env_t global_reader_env;
  expr_t* nil;
  expr_t* t;
  expr_t* void_expr;
  expr_t* chars[128];
  unordered_map<string, expr_t*> char_map;
  unordered_map<string, expr_t*> interned_strings;

  expr_t* read(expr_t* istream);
  expr_t* read(expr_env_t* reader_env, lexer_t& lexer, token_t token);
  expr_t* read_nil(expr_env_t* reader_env);
  expr_t* read_char(expr_env_t* reader_env, token_t token);
  expr_t* read_number(expr_env_t* reader_env, token_t token);
  expr_t* read_string(expr_env_t* reader_env, token_t token);
  expr_t* read_symbol(expr_env_t* reader_env, lexer_t& lexer, token_t token);
  expr_t* read_apostrophe(expr_env_t* reader_env, lexer_t& lexer, token_t token);
  expr_t* read_backquote(expr_env_t* reader_env, lexer_t& lexer, token_t token);
  expr_t* read_list(expr_env_t* reader_env, lexer_t& lexer);

  expr_t* eval(expr_t* expr, expr_env_t* env);

  expr_t* apply(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_primitive_proc(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_special_form(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_compound_proc(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_macro(expr_t* expr, expr_t* args, expr_env_t* env);

  expr_t* macroexpand_all(expr_t* expr, expr_env_t* env);
  expr_t* macroexpand(expr_t* expr, expr_env_t* env);
  expr_t* expand(expr_t* expr, expr_env_t* env);

  expr_t* quasiquote(expr_t* expr, expr_env_t* env);

  // ---

  expr_t* make_void();
  bool is_void(expr_t* expr);

  expr_t* make_nil();
  bool is_nil(expr_t* expr);

  expr_t* make_char(char c);
  bool is_char(expr_t* expr);
  char get_char(expr_t* expr);

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

  expr_t* make_macro(const function<expr_t*(expr_t*)>& f);
  bool is_macro(expr_t* expr);

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

  expr_t* make_istream(istream& is);
  bool is_istream(expr_t* expr);
  istream& get_istream(expr_t* expr);

  expr_t* make_ostream(ostream& os);
  bool is_ostream(expr_t* expr);
  ostream& get_ostream(expr_t* expr);

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
  expr_t* list_reverse(expr_t* expr);

  bool is_tagged(expr_t* expr, expr_t* symbol);
  bool is_tagged(expr_t* expr, const string& symbol);
};

#endif // INTERPRETER_H

