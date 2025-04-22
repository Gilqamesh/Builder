#include "memory.h"

memory_t::memory_t() {
  m_nil = (expr_t*) new expr_nil_t();
  m_void = (expr_t*) new expr_void_t();
  for (int i = 0; i < sizeof(m_chars) / sizeof(m_chars[0]); ++i) {
    m_chars[i] = (expr_t*) new expr_char_t((char)i);
  }
  m_char_map = {
    { "space", m_chars[' '] },
    { "backspace", m_chars[8] },
    { "newline", m_chars['\n'] },
    { "tab", m_chars['\t'] }
  };
  m_t = make_symbol("#t");
}

expr_t* memory_t::make_void() {
  return m_void;
}

expr_t* memory_t::make_nil() {
  return m_nil;
}

expr_t* memory_t::make_char(char c) {
  return m_chars[c];
}

expr_t* memory_t::make_char(const string& symbol) {
  if (symbol.size() == 1) {
    return make_char(symbol[0] );
  }
  auto it = m_char_map.find(symbol);
  if (it == m_char_map.end()) {
    return 0;
  }
  return it->second;
}

expr_t* memory_t::make_boolean(bool boolean) {
  return boolean ? m_t : make_nil();
}

expr_t* memory_t::make_integer(int64_t integer) {
  return (expr_t*) new expr_integer_t(integer);
}

expr_t* memory_t::make_real(double real) {
  return (expr_t*) new expr_real_t(real);
}

expr_t* memory_t::make_string(const string& str) {
  return (expr_t*) new expr_string_t(str);
}

expr_t* memory_t::make_symbol(const string& symbol) {
  auto it = m_interned_symbols.find(symbol);
  if (it != m_interned_symbols.end()) {
    return it->second;
  }
  expr_t* result = (expr_t*) new expr_symbol_t(symbol);
  m_interned_symbols[symbol] = result;
  return result;
}

expr_t* memory_t::make_istream(istream& is) {
  return (expr_t*) new expr_istream_t(is);
}

expr_t* memory_t::make_ostream(ostream& os) {
  return (expr_t*) new expr_ostream_t(os);
}

expr_t* memory_t::make_cons(expr_t* expr1, expr_t* expr2) {
  return (expr_t*) new expr_cons_t(expr1, expr2);
}

expr_t* memory_t::make_compound_proc(expr_t* params, expr_t* body) {
  return (expr_t*) new expr_compound_proc_t(params, body);
}

expr_t* memory_t::make_macro(const function<expr_t*(expr_t*)>& f) {
  return (expr_t*) new expr_macro_t(f);
}

expr_t* memory_t::make_special_form(const function<expr_t*(expr_t*, expr_env_t*)>& f) {
  return (expr_t*) new expr_special_form_t(f);
}

expr_t* memory_t::make_primitive_proc(const function<expr_t*(expr_t*)>& f, int arity, bool is_variadic) {
  return (expr_t*) new expr_primitive_proc_t(f, arity, is_variadic);
}

expr_t* memory_t::make_env() {
  return (expr_t*) new expr_env_t();
}

