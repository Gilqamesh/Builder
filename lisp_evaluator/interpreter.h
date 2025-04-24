#ifndef PARSER_H
# define PARSER_H

# include "expr.h"
# include "memory.h"

struct interpreter_t {
  interpreter_t();

  // always returns a valid expression
  expr_t* read(istream& is, bool recursive = false);
  bool is_eof(expr_t* expr);

  expr_t* eval(expr_t* expr);
  void print(ostream& os, expr_t* expr);

  void source(const char* filepath);
  void source(istream& is);
  void source(ostream& os, istream& is);

private:
  // todo: add system_env where all initial stuff lives, expose user_env to users
  expr_env_t global_env;
  expr_env_t global_reader_env;

  memory_t memory;

  function<expr_t*(istream&, char)> m_reader_macros[256];
  void set_macro_character(char c, function<expr_t*(istream&, char)> f);
  function<expr_t*(istream& is, char)> get_macro_character(char c) const;
 
  bool is_whitespace(char c) const;
  bool is_terminating_macro(char c) const;
  bool is_non_terminating_macro(char c) const;
  bool is_single_escape(char c) const;
  bool is_multiple_escape(char c) const;
  bool is_constituent(char c) const;
  bool is_illegal(char c) const;

  char read_char(istream& is) const;
  bool read_char(istream& is, const string& str) const;
  bool read_char(istream& is, char c) const;
  void unread_char(istream& is, char c) const;
  char peek_char(istream& is) const;
  bool is_at_end(istream& is) const;

  expr_t* read(string& lexeme, istream& is, bool recursive);
  expr_t* read_illegal();
  expr_t* read_whitespace(string& lexeme, istream& is, char c, bool recursive);
  expr_t* read_terminating_macro(string& lexeme, istream& is, char c, bool recursive);
  expr_t* read_non_terminating_macro(string& lexeme, istream& is, char c, bool recursive);
  expr_t* read_single_escape(string& lexeme, istream& is, char c, bool recursive);
  expr_t* read_multiple_escape(string& lexeme, istream& is, char c, bool recursive);
  expr_t* read_constituent(string& lexeme, istream& is, char c, bool recursive);
  
  expr_t* reader_step8(string& lexeme, istream& is, char c, bool recursive);
  expr_t* reader_step9(string& lexeme, istream& is, char c, bool recursive);
  expr_t* reader_step10(string& lexeme);

  expr_t* eval(expr_t* expr, expr_env_t* env);

  expr_t* list_of_values(expr_t* arguments, expr_t* parameters, expr_env_t* env);
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

  expr_t* make_list(const initializer_list<expr_t*>& exprs);
  bool is_list(expr_t* expr);
  expr_t* list_tail(expr_t* expr, int n);
  expr_t* list_tail(expr_t* expr, expr_t* integer_expr);
  expr_t* list_ref(expr_t* expr, int n);
  expr_t* list_ref(expr_t* expr, expr_t* integer_expr);
  int list_length_internal(expr_t* expr);
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

