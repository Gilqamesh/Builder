#include "reader2.h"

reader_t::reader_t(istream& is):
  is(is)
{
  for (int i = 0; i < sizeof(macros) / sizeof(macros[0]); ++i) {
    macros[i] = 0;
  }

  set_macro_character('"', [](reader_t& reader, char c) {
    reader.push(c);
    while (!reader.is_at_end()) {
      char y = reader.eat();
      reader.push(y);
      if (y == '\"') {
        return (expr_t*) new expr_string_t(reader.lexeme());
      }
    }
    throw runtime_error("reader macro \": expect \" at the end of lexeme: '" + reader.lexeme() + "'");
  });
  set_macro_character('\'', [](reader_t& reader, char c) {
    return reader.read(true, 0, true);
  });
  set_macro_character('(', [](reader_t& reader, char c) {
    expr_t* result = 0;
    expr_t* prev = 0;
    while (!reader.is_at_end() && reader.peek() != ')') {
      while (!reader.is_at_end() && reader.is_whitespace(reader.peek())) {
        reader.eat();
      }
      expr_t* cur = (expr_t*) new expr_cons_t(reader.read(true, 0, true), (expr_t*) new expr_nil_t());
      while (!reader.is_at_end() && reader.is_whitespace(reader.peek())) {
        reader.eat();
      }
      if (!result) {
        result = cur;
      } else {
        ((expr_cons_t*)prev)->second = cur;
      }
      prev = cur;
    }
    if (reader.is_at_end()) {
      throw runtime_error("reahed eof, expect ')'");
    }
    if (reader.eat() != ')') {
      throw runtime_error("expect ')'");
    }
    if (result) {
      return result;
    }
    return (expr_t*) new expr_nil_t();
  });
}

expr_t* reader_t::read(bool eof_error_p, expr_t* eof_value, bool recursive) {
  if (is_at_end()) {
    if (eof_error_p) {
      throw runtime_error("");
    } else {
      return eof_value;
    }
  }

  clear();
  unsigned char c = (unsigned char) eat();
  if (is_whitespace(c)) {
    return read_whitespace(c, eof_error_p, eof_value, recursive);
  } else if (is_terminating_macro(c)) {
    return read_terminating_macro(c, eof_error_p, eof_value, recursive);
  } else if (is_non_terminating_macro(c)) {
    return read_non_terminating_macro(c, eof_error_p, eof_value, recursive);
  } else if (is_single_escape(c)) {
    return read_single_escape(c, eof_error_p, eof_value, recursive);
  } else if (is_multiple_escape(c)) {
    return read_multiple_escape(c, eof_error_p, eof_value, recursive);
  } else if (is_constituent(c)) {
    return read_constituent(c, eof_error_p, eof_value, recursive);
  } else {
    assert(is_illegal(c));
    return read_illegal(c, eof_error_p, eof_value, recursive);
  }
}

void reader_t::set_macro_character(char c, function<expr_t*(reader_t&, char)> f) {
  macros[(unsigned char)c] = f;
}

function<expr_t*(reader_t&, char)> reader_t::get_macro_character(char c) const {
  function<expr_t*(reader_t&, char)> result = macros[(unsigned char)c];
  if (!result) {
    throw runtime_error("not implemented macro character " + string(1, c));
  }
  return result;
}

char reader_t::eat() {
  return is.get();
}

void reader_t::uneat(char c) {
  is.putback(c);
}

char reader_t::peek() const {
  return is.peek();
}

bool reader_t::is_at_end() const {
  return is && is.peek() == EOF;
}

void reader_t::push(char c) {
  m_lexeme.push_back(c);
}

char reader_t::pop() {
  char c = back();
  m_lexeme.pop_back();
  return c;
}

void reader_t::clear() {
  m_lexeme.clear();
}

char reader_t::back() const {
  if (m_lexeme.empty()) {
    throw runtime_error("back: empty lexeme");
  }
  return m_lexeme.back();
}

const string& reader_t::lexeme() const {
  return m_lexeme;
}

bool reader_t::is_illegal(char c) const {
  return !is_whitespace(c) || !is_terminating_macro(c) || !is_non_terminating_macro(c) || !is_single_escape(c) || !is_multiple_escape(c) || !is_constituent(c);
}

bool reader_t::is_whitespace(char c) const {
  return c == '\t' || c == ' ' || c == '\f' || c == '\r' || c == '\n' || c == '\v';
}

bool reader_t::is_terminating_macro(char c) const {
  return c == '"' || c == '\'' || c == '(' || c == ')' || c == ',' || c == ';' || c == '`';
}

bool reader_t::is_non_terminating_macro(char c) const {
  return c == '#';
}

bool reader_t::is_single_escape(char c) const {
  return c == '\\';
}

bool reader_t::is_multiple_escape(char c) const {
  return c == '|';
}

bool reader_t::is_constituent(char c) const {
  return c == '!' || c == '$' || c == '%' || c == '&' || c == '*' || c == '+' || c == '-' || c == '.' || c == '/' || c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' ||
         c == '6' || c == '7' || c == '8' || c == '9' || c == ':' || c == '<' || c == '=' || c == '>' || c == '?' || c == '\b' || c == '@' || c == 'A' || c == 'B' || c == 'C' || c == 'D' ||
         c == 'E' || c == 'F' || c == 'G' || c == 'H' || c == 'I' || c == 'J' || c == 'K' || c == 'L' || c == 'M' || c == 'N' || c == 'O' || c == 'P' || c == 'Q' || c == 'R' || c == 'S' ||
         c == 'T' || c == 'U' || c == 'V' || c == 'W' || c == 'X' || c == 'Y' || c == 'Z' || c == '[' || c == ']' || c == '^' || c == '_' || c == 'a' || c == 'b' || c == 'c' || c == 'd' ||
         c == 'e' || c == 'f' || c == 'g' || c == 'h' || c == 'i' || c == 'j' || c == 'k' || c == 'l' || c == 'm' || c == 'n' || c == 'o' || c == 'p' || c == 'q' || c == 'r' || c == 's' ||
         c == 't' || c == 'u' || c == 'v' || c == 'w' || c == 'x' || c == 'y' || c == 'z' || c == '{' || c == '}' || c == '~' || c == 127;
}

expr_t* reader_t::read_illegal(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  throw runtime_error("illegal character");
  return 0;
}

expr_t* reader_t::read_whitespace(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  if (recursive) {
    push(c);
  }
  return read(eof_error_p, eof_value, recursive);
}

expr_t* reader_t::read_terminating_macro(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  expr_t* result = get_macro_character(c)(*this, c);
  if (result) {
    return result;
  }
  return read(eof_error_p, eof_value, recursive);
}

expr_t* reader_t::read_non_terminating_macro(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  expr_t* result = get_macro_character(c)(*this, c);
  if (result) {
    return result;
  }
  return read(eof_error_p, eof_value, recursive);
}

expr_t* reader_t::read_single_escape(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  if (is_at_end()) {
    throw runtime_error("single escape: end of file, expect character");
  }
  char y = eat();
  clear();
  push(y);
  return step8(y, eof_error_p, eof_value, recursive);
}

expr_t* reader_t::read_multiple_escape(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  clear();
  return step9(c, eof_error_p, eof_value, recursive);
}

expr_t* reader_t::read_constituent(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  clear();
  push(c);
  return step8(c, eof_error_p, eof_value, recursive);
}

expr_t* reader_t::step8(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  if (is_at_end()) {
    return step10(c, eof_error_p, eof_value, recursive);
  }
  char y = eat();
  if (is_constituent(y) || is_non_terminating_macro(y)) {
    push(y);
    return step8(y, eof_error_p, eof_value, recursive);
  } else if (is_single_escape(y)) {
    if (is_at_end()) {
      throw runtime_error("expect char");
    }
    char z = eat();
    push(z);
    return step8(z, eof_error_p, eof_value, recursive);
  } else if (is_multiple_escape(y)) {
    return step9(y, eof_error_p, eof_value, recursive);
  } else if (is_terminating_macro(y)) {
    uneat(y);
    return step10(c, eof_error_p, eof_value, recursive);
  } else if (is_whitespace(y)) {
    if (!recursive) {
      uneat(y);
    }
    return step10(y, eof_error_p, eof_value, recursive);
  } else {
    assert(is_illegal(y));
    throw runtime_error("illegal character");
    return 0;
  }
}

expr_t* reader_t::step9(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  if (is_at_end()) {
    throw runtime_error("eof");
  }

  char y = eat();
  if (is_constituent(y) || is_terminating_macro(y) || is_non_terminating_macro(y) || is_whitespace(y)) {
    push(y);
    return step9(y, eof_error_p, eof_value, recursive);
  } else if (is_single_escape(y)) {
    if (is_at_end()) {
      throw runtime_error("eof");
    }
    char z = eat();
    push(z);
    return step9(z, eof_error_p, eof_value, recursive);
  } else if (is_multiple_escape(y)) {
    return step8(y, eof_error_p, eof_value, recursive);
  } else {
    assert(is_illegal(y));
    throw runtime_error("illegal character");
    return 0;
  }
}

expr_t* reader_t::step10(char c, bool eof_error_p, expr_t* eof_value, bool recursive) {
  expr_t* result = (expr_t*) new expr_symbol_t(m_lexeme);
  clear();
  return result;
}

