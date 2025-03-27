#ifndef LEXER_H
# define LEXER_H

# include "libc.h"

enum class token_type_t : int {
  LEFT_PAREN,
  RIGHT_PAREN,
  APOSTROPHE,
  ELLIPSIS,

  // SPECIAL FORMS
  DEFINE,
  LAMBDA,
  IF,
  ELSE,
  WHEN,
  COND,
  LET,
  BEGIN,
  QUOTE,
  SET,

  // LITERALS
  IDENTIFIER,
  NUMBER,
  STRING,
  NIL,

  // SPECIAL
  ERROR,
  COMMENT,
  END_OF_FILE,

  _SIZE
};
const char* token_type_to_str(token_type_t type);

struct token_t {
  const char*  lexeme; // not null terminated, could move to std::string
  const char*  error; // null terminated
  int          lexeme_length;
  int          error_line;
  int          error_col;
  int          line_start;
  int          line_end;
  int          col_start;
  int          col_end;
  token_type_t type;

  string to_string() const;

  friend ostream& operator<<(ostream& os, const token_t& token);
};

struct token_exception_t : public exception {
  token_exception_t(const string& message, token_t token);

  token_t token;
  string message;

  const char* what() const noexcept override;
};

struct lexer_t {
  lexer_t(const char* source);

  token_t eat_token();

private:
  const char* start;
  const char* end;
  int         line_start;
  int         line_end;
  int         col_start;
  int         col_end;

  char peak_char(int ahead = 0) const;
  char eat_char();
  bool is_at_end() const;
  void skip_whitespaces();
  bool is_whitespace(char c) const;

  token_t make_token(token_type_t type);
  token_t make_error_token(const char* err_msg, int line, int col);
  token_t eat_number(bool dotted);
  token_t eat_identifier_if(const char* match, token_type_t type);
  token_t eat_identifier();
  token_t eat_comment();
  token_t eat_string();
};

#endif // LEXER_H

