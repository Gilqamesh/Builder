#include "lexer.h"

const char* token_type_to_str(token_type_t type) {
  switch (type) {
  case token_type_t::LEFT_PAREN: return "LEFT_PAREN";
  case token_type_t::RIGHT_PAREN: return "RIGHT_PAREN";
  case token_type_t::APOSTROPHE: return "APOSTROPHE";
  case token_type_t::ELLIPSIS: return "ELLIPSIS";
  case token_type_t::DEFINE: return "DEFINE";
  case token_type_t::LAMBDA: return "LAMBDA";
  case token_type_t::IF: return "IF";
  case token_type_t::ELSE: return "ELSE";
  case token_type_t::WHEN: return "WHEN";
  case token_type_t::COND: return "COND";
  case token_type_t::LET: return "LET";
  case token_type_t::BEGIN: return "BEGIN";
  case token_type_t::QUOTE: return "QUOTE";
  case token_type_t::SET: return "SET!";
  case token_type_t::IDENTIFIER: return "IDENTIFIER";
  case token_type_t::NUMBER: return "NUMBER";
  case token_type_t::STRING: return "STRING";
  case token_type_t::NIL: return "NIL";
  case token_type_t::ERROR: return "ERROR";
  case token_type_t::COMMENT: return "COMMENT";
  case token_type_t::END_OF_FILE: return "END_OF_FILE";
  default: assert(0);
  }
}

string token_t::to_string() const {
  return string(lexeme, (size_t) lexeme_length);
}

ostream& operator<<(ostream& os, const token_t& token) {
  os << string(token_type_to_str(token.type)) << " " << token.line_start << ":" << token.col_start;
  if (token.type != token_type_t::END_OF_FILE) {
    os << " '" << token.to_string() << "'";
  }
  if (token.type == token_type_t::ERROR) {
    os << ", " << token.error_line << ":" << token.error_col << ": " << token.error;
  }
  return os;
}

token_exception_t::token_exception_t(const string& message, token_t token):
  message(message),
  token(token)
{
}

const char* token_exception_t::what() const noexcept {
  return message.c_str();
}

lexer_t::lexer_t(const char* source) {
  start = source;
  end = source;
  line_start = 0;
  line_end = 0;
  col_start = 0;
  col_end = 0;
}

token_t lexer_t::eat_token() {
  skip_whitespaces();

  start = end;
  line_start = line_end;
  col_start = col_end;

  if (is_at_end()) {
    return make_token(token_type_t::END_OF_FILE);
  }

  switch (eat_char()) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': return eat_number(false);
  case '.': {
    if ('0' <= peak_char() && peak_char() <= '9') {
      return eat_number(true);
    }
    return eat_identifier();
  } break ;
  case '(': return make_token(token_type_t::LEFT_PAREN);
  case ')': return make_token(token_type_t::RIGHT_PAREN);
  case '\'': return make_token(token_type_t::APOSTROPHE);
  case '"': return eat_string();
  case '#': {
    if (peak_char() == '|') {
      return eat_comment();
    } else {
      return eat_identifier();
    }
  } break ;
  default: return eat_identifier();
  }
}

bool lexer_t::is_at_end() const {
  return peak_char() == '\0';
}

char lexer_t::peak_char(int ahead) const {
  const char* cur = end;
  while (*cur != '\0' && ahead) {
    ++cur;
    --ahead;
  }
  return *cur;
}

char lexer_t::eat_char() {
  ++col_end;
  if (*end == '\n') {
    ++line_end;
    col_end = 0;
  }
  if (*end == '\r' && peak_char(1) == '\n') {
    ++end;
    ++line_end;
    col_end = 0;
  }
  return *end++;
}

void lexer_t::skip_whitespaces() {
  while (!is_at_end() && is_whitespace(peak_char())) {
    eat_char();
  }
}

bool lexer_t::is_whitespace(char c) const {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

token_t lexer_t::make_token(token_type_t type) {
  return token_t {
    .lexeme = start,
    .error = "",
    .lexeme_length = (int)(end - start),
    .error_line = 0,
    .error_col = 0,
    .line_start = line_start,
    .line_end = line_end,
    .col_start = col_start,
    .col_end = col_end,
    .type = type
  };
}

token_t lexer_t::make_error_token(const char* err_msg, int line, int col) {
  token_t result = make_token(token_type_t::ERROR);
  result.error = err_msg;
  result.error_line = line;
  result.error_col = col;
  return result;
}

token_t lexer_t::eat_number(bool dotted) {
  while (!is_at_end() && '0' <= peak_char() && peak_char() <= '9') {
    eat_char();
  }
  if (!dotted) {
    if (peak_char() == '.') {
      eat_char();
    }
  }
  while (!is_at_end() && '0' <= peak_char() && peak_char() <= '9') {
    eat_char();
  }
  if (is_at_end() || is_whitespace(peak_char())) {
    return make_token(token_type_t::NUMBER);
  }
  int err_line = line_end;
  int err_col = col_end;
  while (!is_at_end() && !is_whitespace(peak_char()) && peak_char() != '(' && peak_char() != ')') {
    eat_char();
  }
  return make_error_token("expect EOF or whitespace character", err_line, err_col);
}

token_t lexer_t::eat_identifier_if(const char* match, token_type_t type) {
  const char* cur = start;
  while (cur < end && *match != '\0') {
    if (*cur != *match) {
      break ;
    }
    ++cur;
    ++match;
  }
  if (cur == end && *match == '\0') {
    return make_token(type);
  }
  return make_token(token_type_t::IDENTIFIER);
}

token_t lexer_t::eat_identifier() {
  while (!is_at_end() && !is_whitespace(peak_char()) && peak_char() != '(' && peak_char() != ')') {
    eat_char();
  }

  switch (*start) {
  case '.': return eat_identifier_if("...", token_type_t::ELLIPSIS);
  case 'd': return eat_identifier_if("define", token_type_t::DEFINE);
  case 'l': {
    switch (*(start + 1)) {
    case 'a': return eat_identifier_if("lambda", token_type_t::LAMBDA);
    case 'e': return eat_identifier_if("let", token_type_t::LET);
    }
  } break ;
  case 's': return eat_identifier_if("set!", token_type_t::SET);
  case 'f': return eat_identifier_if("nil", token_type_t::NIL);
  case 'i': return eat_identifier_if("if", token_type_t::IF);
  case 'e': return eat_identifier_if("else", token_type_t::ELSE);
  case 'c': return eat_identifier_if("cond", token_type_t::COND);
  case 'b': return eat_identifier_if("begin", token_type_t::BEGIN);
  case 'w': return eat_identifier_if("when", token_type_t::WHEN);
  case 'q': return eat_identifier_if("quote", token_type_t::QUOTE);
  }
  return make_token(token_type_t::IDENTIFIER);
}

token_t lexer_t::eat_comment() {
  while (!is_at_end()) {
    if (eat_char() == '|') {
      if (!is_at_end() && eat_char() == '#') {
        return make_token(token_type_t::COMMENT);
      }
    }
  }
  return make_error_token("expect |# for end of comment", line_end, col_end);
}

token_t lexer_t::eat_string() {
  while (!is_at_end()) {
    if (eat_char() == '"') {
      return make_token(token_type_t::STRING);
    }
  }
  return make_error_token("expect \" for end of string", line_end, col_end);
}

