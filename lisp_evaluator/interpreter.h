#ifndef PARSER_H
# define PARSER_H

# include "expr.h"
# include "memory.h"

struct interpreter_t {
  interpreter_t();

  // always returns a valid expression
  expr_t* read(istream& is);

  expr_t* eval(expr_t* expr);
  void print(ostream& os, expr_t* expr);

  void source(const char* filepath);
  void source(istream& is);
  void source(ostream& os, istream& is);
  void repl();

private:
  size_t m_total_eval_cy = 0;
  size_t m_total_apply_cy = 0;
  size_t m_total_compile_cy = 0;

  // todo: add system_env where all initial stuff lives, expose user_env to users
  expr_env_t global_env;
  expr_env_t global_reader_env;
  memory_t memory;

  struct node_t {
    node_t();

    node_t* children[128];
    function<expr_t*(istream&)> reader_macro;
  };
  bool m_whitespaces[128];
  node_t m_reader_macro_root;

  void register_symbols();

  void register_reader_macro(const string& prefix, function<expr_t*(istream&)> f);
  void register_reader_macros();

  expr_t* register_macro(expr_env_t* env, const string& name, function<expr_t*(expr_t*, expr_env_t*)> f);
  void register_macros();

  expr_t* register_primitive_proc(expr_env_t* env, const string& name, function<expr_t*(expr_t*, expr_env_t*)> f);
  void register_primitive_procs();

  expr_t* register_special_form(expr_env_t* env, const string& name, function<expr_t*(expr_t*, expr_env_t*)> f);
  void register_special_forms();
 
  bool is_whitespace(char c) const;
  char read_char(istream& is) const;
  bool read_char(istream& is, const string& str) const;
  bool read_char(istream& is, char c) const;
  void unread_char(istream& is, char c) const;
  char peek_char(istream& is) const;
  bool is_at_end(istream& is) const;

  expr_t* default_reader_macro(istream& is);
  expr_t* read_whitespaces(istream& is);

  expr_t* eval(expr_t* expr, expr_env_t* env);

  expr_t* bind_params(expr_t* args, expr_t* params);
  expr_t* list_of_values(expr_t* arguments, expr_t* parameters, expr_env_t* env);
  expr_t* begin(expr_t* expr, expr_env_t* env);

  expr_t* apply(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_primitive_proc(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_special_form(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_compound_proc(expr_t* expr, expr_t* args, expr_env_t* env);
  expr_t* apply_macro(expr_t* expr, expr_t* args, expr_env_t* env);

  expr_t* macroexpand_all(expr_t* expr, expr_env_t* env);
  expr_t* macroexpand(expr_t* expr, expr_env_t* env);
  expr_t* expand(expr_t* expr, expr_env_t* env);

  void quasiquote(expr_t* expr, expr_env_t* env, expr_t* car_parent);
  bool is_unquote(expr_t* expr);
  bool is_unquote_splicing(expr_t* expr);

  expr_t* compile(expr_t* expr);
  expr_t* compile_void_exprs_out(expr_t* expr, unordered_set<expr_t*>& seen);

  // ---

  expr_t* number_add(expr_t* a, expr_t* b);
  expr_t* number_sub(expr_t* a, expr_t* b);
  expr_t* number_mul(expr_t* a, expr_t* b);
  expr_t* number_div(expr_t* a, expr_t* b);
  expr_t* number_eq(expr_t* a, expr_t* b);

  expr_env_t* extend_env(expr_env_t* env, expr_t* symbols, expr_t* exprs);

  expr_t* cons(expr_t* expr1, expr_t* expr2);

  // ---

  bool is_eq(expr_t* expr1, expr_t* expr2);
  bool is_true(expr_t* expr);
  bool is_false(expr_t* expr);

  expr_t* make_list(const initializer_list<expr_t*>& exprs);
  bool is_list(expr_t* expr);
  expr_t* list_tail(expr_t* expr, int n);
  expr_t* list_tail(expr_t* expr, expr_t* integer_expr);
  expr_t* list_ref(expr_t* expr, int n);
  expr_t* list_ref(expr_t* expr, expr_t* integer_expr);
  size_t list_length_internal(expr_t* expr);
  expr_t* list_length(expr_t* expr);
  expr_t* list_map(expr_t* expr, const function<expr_t*(expr_t*)>& f);
  expr_t* list_reverse(expr_t* expr);
  expr_t* list_copy(expr_t* l);
  expr_t* list_append(expr_t* l, expr_t* e, bool pure);

  bool is_tagged(expr_t* expr, expr_t* symbol);
  bool is_tagged(expr_t* expr, const string& symbol);

  expr_t* dot();
};

#endif // INTERPRETER_H

