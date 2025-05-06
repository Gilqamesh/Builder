#include "memory.h"

memory_t::memory_t() {
  m_eof = (expr_t*) new expr_eof_t();
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
  m_t = (expr_t*) new expr_boolean_t(true);
  m_f = (expr_t*) new expr_boolean_t(false);
}

expr_t* memory_t::make_eof() {
  return m_eof;
}

expr_t* memory_t::make_nil() {
  return m_nil;
}

expr_t* memory_t::make_void() {
  return m_void;
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
  if (boolean) {
    return m_t;
  } else {
    return m_f;
  }
}

expr_t* memory_t::make_integer(int64_t integer) {
  return (expr_t*) new expr_integer_t(integer);
}

expr_t* memory_t::make_real(double real) {
  if (real == (int64_t)real) {
    return make_integer((int64_t)real);
  }
  return (expr_t*) new expr_real_t(real);
}

expr_t* memory_t::make_string(const string& str) {
  return (expr_t*) new expr_string_t(str);
}

expr_t* memory_t::make_symbol(const string& symbol, bool is_interned) {
  auto it = m_interned_symbols.find(symbol);
  if (it != m_interned_symbols.end()) {
    return it->second;
  }
  expr_t* result = (expr_t*) new expr_symbol_t(symbol);
  if (is_interned) {
    m_interned_symbols[symbol] = result;
  }
  return result;
}

expr_t* memory_t::make_istream(istream& is) {
  return (expr_t*) new expr_istream_t(is);
}

expr_t* memory_t::make_istream(unique_ptr<istream> is) {
  return (expr_t*) new expr_istream_t(std::move(is));
}

expr_t* memory_t::make_ostream(ostream& os) {
  return (expr_t*) new expr_ostream_t(os);
}

expr_t* memory_t::make_ostream(unique_ptr<ostream> os) {
  return (expr_t*) new expr_ostream_t(std::move(os));
}

expr_t* memory_t::make_cons(expr_t* expr1, expr_t* expr2) {
  return (expr_t*) new expr_cons_t(expr1, expr2);
}

expr_t* memory_t::make_macro(function<expr_t*(expr_t* args, expr_t* call_env)> f) {
  return (expr_t*) new expr_macro_t(f);
}

expr_t* memory_t::make_env() {
  return (expr_t*) new expr_env_t();
}

expr_t* memory_t::shallow_copy(expr_t* expr) {
  if (!is_cons(expr)) {
    return expr;
  }
  return make_cons(shallow_copy(car(expr)), shallow_copy(cdr(expr)));
}

