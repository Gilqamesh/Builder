#ifndef READER_H
# define READER_H

# include "expr.h"

struct reader_t {
  reader_t(istream& is, function<expr_t*(const string&)> default_lexeme_handler);

  void set_skippable_predicate(function<bool(char)> skippable_predicate);

  void add(const string& lexeme, function<expr_t*(reader_t&)> f);

  expr_t* read(function<bool(reader_t&)> terminator);
  expr_t* read();

  char peek() const;
  char eat();
  bool is_at_end() const;
  bool is_skippable(char c) const;

  const string lexeme() const;

private:
  struct node_t {
    node_t();
    node_t* children[128];
    function<expr_t*(reader_t& reader)> f;
  };
 
  node_t root;
  istream& is;

  stack<function<bool(reader_t&)>> terminators;

  function<bool(char)> skippable_predicate;
  function<expr_t*(const string&)> default_lexeme_handler;

  string m_lexeme;
};

#endif // READER_H

