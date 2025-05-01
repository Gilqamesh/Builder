#ifndef MEMORY_H
# define MEMORY_H

# include "expr.h"

struct memory_t {
  memory_t();

  expr_t* make_eof();
  expr_t* make_void();
  expr_t* make_nil();
  expr_t* make_boolean(bool boolean);
  expr_t* make_char(char c);
  expr_t* make_char(const string& symbol);
  expr_t* make_integer(int64_t integer);
  expr_t* make_real(double real);
  expr_t* make_string(const string& str);
  expr_t* make_symbol(const string& symbol, bool is_interned = true);
  expr_t* make_istream(istream& is);
  expr_t* make_istream(unique_ptr<istream> is);
  expr_t* make_ostream(ostream& os);
  expr_t* make_ostream(unique_ptr<ostream> os);
  expr_t* make_cons(expr_t* expr1, expr_t* expr2);
  expr_t* make_compound_proc(expr_t* params, expr_t* body);
  expr_t* make_macro(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f);
  expr_t* make_special_form(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f);
  expr_t* make_primitive_proc(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f);
  expr_t* make_env();

  expr_t* shallow_copy(expr_t* expr);

private:
  expr_t* m_eof;
  expr_t* m_void;
  expr_t* m_nil;
  expr_t* m_t;
  expr_t* m_f;
  expr_t* m_chars[256];
  unordered_map<string, expr_t*> m_char_map;
  unordered_map<string, expr_t*> m_interned_symbols;
};

#endif // memory_h

