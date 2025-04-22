#ifndef READER_H
# define READER_H

# include "expr.h"

char eat(istream& is);
void uneat(istream& is, char c);
char peek(istream& is);
bool is_at_end(istream& is);

bool is_illegal(char c) const;
bool is_whitespace(char c) const;
bool is_terminating_macro(char c) const;
bool is_non_terminating_macro(char c) const;
bool is_single_escape(char c) const;
bool is_multiple_escape(char c) const;
bool is_constituent(char c) const;

struct reader_t {
  reader_t();

  // either returns a valid expr or throws, always call with recursive = true inside a reader macro
  expr_t* read(istream& is, bool recursive = false);

  void set_macro_character(char c, function<expr_t*(istream&, char)> f);
  function<expr_t*(reader_t&, istream& is, char)> get_macro_character(char c) const;

  // character types
  // todo: move outside
private:
  function<expr_t*(reader_t&, char)> macros[256];
};

#endif // READER

