#include "lexer.h"

ostream& operator<<(ostream& os, const token_t& token) {
  os << token_type_to_str(token.type) << " " << token.line_start << ":" << token.col_start;
  if (token.type != token_type_t::END_OF_FILE) {
    os << " '";
    for (int i = 0; i < token.lexeme_length; ++i) {
      os << token.lexeme[i];
    }
    os << "'";
  }
  if (token.type == token_type_t::ERROR) {
    os << ", " << token.error_line << ":" << token.error_col << ": " << token.error;
  }

  return os;
}

const char* token_type_to_str(token_type_t type) {
  switch (type) {
  case token_type_t::LEFT_PAREN: return "LEFT_PAREN";
  case token_type_t::RIGHT_PAREN: return "RIGHT_PAREN";
  case token_type_t::APOSTROPHE: return "APOSTROPHE";
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
  case token_type_t::FALSE: return "FALSE";
  case token_type_t::CONS: return "CONS";
  case token_type_t::CAR: return "CAR";
  case token_type_t::CDR: return "CDR";
  case token_type_t::LIST: return "LIST";
  case token_type_t::ERROR: return "ERROR";
  case token_type_t::COMMENT: return "COMMENT";
  case token_type_t::END_OF_FILE: return "END_OF_FILE";
  default: assert(0);
  }
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
  case '9': return make_number(false);
  case '.': {
    if ('0' <= peak_char() && peak_char() <= '9') {
      return make_number(true);
    }
    return make_identifier();
  } break ;
  case '(': return make_token(token_type_t::LEFT_PAREN);
  case ')': return make_token(token_type_t::RIGHT_PAREN);
  case '\'': return make_token(token_type_t::APOSTROPHE);
  case '"': return make_string();
  case '#': {
    if (peak_char() == '|') {
      return make_comment();
    } else {
      return make_identifier();
    }
  } break ;
  default: return make_identifier();
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

token_t lexer_t::make_number(bool dotted) {
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

token_t lexer_t::make_identifier_if(const char* match, token_type_t type) {
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

token_t lexer_t::make_identifier() {
  while (!is_at_end() && !is_whitespace(peak_char()) && peak_char() != '(' && peak_char() != ')') {
    eat_char();
  }

  switch (*start) {
  case 'd': return make_identifier_if("define", token_type_t::DEFINE);
  case 'l': {
    switch (*(start + 1)) {
    case 'a': return make_identifier_if("lambda", token_type_t::LAMBDA);
    case 'e': return make_identifier_if("let", token_type_t::LET);
    case 'i': return make_identifier_if("list", token_type_t::LIST);
    }
  } break ;
  case 's': return make_identifier_if("set!", token_type_t::SET);
  case 'f': return make_identifier_if("false", token_type_t::FALSE);
  case 'i': return make_identifier_if("if", token_type_t::IF);
  case 'e': return make_identifier_if("else", token_type_t::ELSE);
  case 'c': {
    switch (*(start + 1)) {
      case 'o': {
        if (start + 2 < end && *(start + 2) == 'n') {
          if (start + 3 < end) {
            switch (*(start + 3)) {
            case 's': return make_identifier_if("cons", token_type_t::CONS);
            case 'd': return make_identifier_if("cond", token_type_t::COND);
            }
          }
        }
      } break ;
      case 'a': return make_identifier_if("car", token_type_t::CAR);
      case 'd': return make_identifier_if("cdr", token_type_t::CDR);
    }
  } break ;
  case 'b': return make_identifier_if("begin", token_type_t::BEGIN);
  case 'w': return make_identifier_if("when", token_type_t::WHEN);
  case 'q': return make_identifier_if("quote", token_type_t::QUOTE);
  }
  return make_token(token_type_t::IDENTIFIER);
}

token_t lexer_t::make_comment() {
  while (!is_at_end()) {
    if (eat_char() == '|') {
      if (!is_at_end() && eat_char() == '#') {
        return make_token(token_type_t::COMMENT);
      }
    }
  }
  return make_error_token("expect |# for end of comment", line_end, col_end);
}

token_t lexer_t::make_string() {
  while (!is_at_end()) {
    if (eat_char() == '"') {
      return make_token(token_type_t::STRING);
    }
  }
  return make_error_token("expect \" for end of string", line_end, col_end);
}

