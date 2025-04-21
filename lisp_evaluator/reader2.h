#ifndef READER_H
# define READER_H

# include "expr.h"

struct reader_t {
  reader_t(istream& is);

  // either returns a valid expr or throws, always call with recursive = true inside a reader macro
  expr_t* read(bool eof_error_p = true, expr_t* eof_value = 0, bool recursive = false);

  void set_macro_character(char c, function<expr_t*(reader_t&, char)> f);
  function<expr_t*(reader_t&, char)> get_macro_character(char c) const;

  // stream functions
  char eat();
  void uneat(char c);
  char peek() const;
  bool is_at_end() const;

  // lexeme builder
  void push(char c);
  char pop();
  void clear();
  char back() const;
  const string& lexeme() const;

  // character types
  bool is_illegal(char c) const;
  bool is_whitespace(char c) const;
  bool is_terminating_macro(char c) const;
  bool is_non_terminating_macro(char c) const;
  bool is_single_escape(char c) const;
  bool is_multiple_escape(char c) const;
  bool is_constituent(char c) const;

private:
  istream& is;
  string m_lexeme;

  function<expr_t*(reader_t&, char)> macros[256];

  expr_t* read_illegal(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* read_whitespace(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* read_terminating_macro(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* read_non_terminating_macro(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* read_single_escape(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* read_multiple_escape(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* read_constituent(char c, bool eof_error_p, expr_t* eof_value, bool recursive);

  expr_t* step8(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* step9(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
  expr_t* step10(char c, bool eof_error_p, expr_t* eof_value, bool recursive);
};

#endif // READER

