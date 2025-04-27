#include "interpreter.h"

bool interpreter_t::is_whitespace(char c) const {
  return c == '\t' || c == ' ' || c == '\f' || c == '\r' || c == '\n' || c == '\v';
}

bool interpreter_t::is_terminating_macro(char c) const {
  return c == '"' || c == '\'' || c == '(' || c == ')' || c == ',' || c == ';' || c == '`';
}

bool interpreter_t::is_non_terminating_macro(char c) const {
  return c == '#';
}

bool interpreter_t::is_single_escape(char c) const {
  return c == '\\';
}

bool interpreter_t::is_multiple_escape(char c) const {
  return c == '|';
}

bool interpreter_t::is_constituent(char c) const {
  return c == '!' || c == '$' || c == '%' || c == '&' || c == '*' || c == '+' || c == '-' || c == '.' || c == '/' || c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' ||
         c == '6' || c == '7' || c == '8' || c == '9' || c == ':' || c == '<' || c == '=' || c == '>' || c == '?' || c == '\b' || c == '@' || c == 'A' || c == 'B' || c == 'C' || c == 'D' ||
         c == 'E' || c == 'F' || c == 'G' || c == 'H' || c == 'I' || c == 'J' || c == 'K' || c == 'L' || c == 'M' || c == 'N' || c == 'O' || c == 'P' || c == 'Q' || c == 'R' || c == 'S' ||
         c == 'T' || c == 'U' || c == 'V' || c == 'W' || c == 'X' || c == 'Y' || c == 'Z' || c == '[' || c == ']' || c == '^' || c == '_' || c == 'a' || c == 'b' || c == 'c' || c == 'd' ||
         c == 'e' || c == 'f' || c == 'g' || c == 'h' || c == 'i' || c == 'j' || c == 'k' || c == 'l' || c == 'm' || c == 'n' || c == 'o' || c == 'p' || c == 'q' || c == 'r' || c == 's' ||
         c == 't' || c == 'u' || c == 'v' || c == 'w' || c == 'x' || c == 'y' || c == 'z' || c == '{' || c == '}' || c == '~' || c == 127;
}

bool interpreter_t::is_illegal(char c) const {
  return !is_whitespace(c) || !is_terminating_macro(c) || !is_non_terminating_macro(c) || !is_single_escape(c) || !is_multiple_escape(c) || !is_constituent(c);
}

char interpreter_t::read_char(istream& is) const {
  return is.get();
}

bool interpreter_t::read_char(istream& is, const string& str) const {
  for (int i = 0; i < str.size(); ++i) {
    bool rewind = false;
    if (is_at_end(is) || !read_char(is, str[i])) {
      rewind = true;
    }
    if (rewind) {
      while (0 < i) {
        unread_char(is, str[--i]);
      }
      return false;
      break ;
    }
  }
  return true;
}

bool interpreter_t::read_char(istream& is, char c) const {
  char y = read_char(is);
  if (y != c) {
    unread_char(is, y);
    return false;
  }
  return true;
}

void interpreter_t::unread_char(istream& is, char c) const {
  is.putback(c);
}

char interpreter_t::peek_char(istream& is) const {
  return is.peek();
}

bool interpreter_t::is_at_end(istream& is) const {
  return !is || is.peek() == EOF;
}

interpreter_t::interpreter_t()
{
  for (int i = 0; i < sizeof(m_reader_macros) / sizeof(m_reader_macros[0]); ++i) {
    m_reader_macros[i] = 0;
  }

  set_macro_character('"', [this](istream& is, char c) {
    string lexeme;
    while (!is_at_end(is)) {
      char y = read_char(is);
      lexeme.push_back(y);
      if (y == '\"') {
        lexeme.pop_back();
        return (expr_t*) new expr_string_t(lexeme);
      }
    }
    throw runtime_error("reader macro \": expect \" at the end of lexeme: '" + lexeme + "'");
  });
  set_macro_character('\'', [this](istream& is, char c) {
    return make_list({ memory.make_symbol("quote"), read(is, true) });
  });
  set_macro_character('`', [this](istream& is, char c) {
    return make_list({ memory.make_symbol("quasiquote"), read(is, true) });
  });
  set_macro_character(',', [this](istream& is, char c) {
    if (is_at_end(is)) {
      throw runtime_error("reader macro ',': unexpected eof");
    }
    char y = read_char(is);
    switch (y) {
      case '@': {
        return make_list({ memory.make_symbol("unquote-splicing"), read(is, true) });
      } break ;
      default: {
        unread_char(is, y);
        return make_list({ memory.make_symbol("unquote"), read(is, true) });
      }
    }
  });
  set_macro_character('(', [this](istream& is, char c) {
    expr_t* result = 0;
    expr_t* prev = 0;
    while (!is_at_end(is) && peek_char(is) != ')') {
      while (!is_at_end(is) && is_whitespace(peek_char(is))) {
        read_char(is);
      }
      expr_t* cur = cons(read(is, true), memory.make_nil());
      while (!is_at_end(is) && is_whitespace(peek_char(is))) {
        read_char(is);
      }
      if (!result) {
        result = cur;
      } else {
        ((expr_cons_t*)prev)->second = cur;
      }
      prev = cur;
    }
    if (is_at_end(is)) {
      throw runtime_error("reached eof, expect ')'");
    }
    if (read_char(is) != ')') {
      throw runtime_error("expect ')'");
    }
    if (result) {
      return result;
    }
    return memory.make_nil();
  });

  set_macro_character('#', [this](istream& is, char c) {
    if (is_at_end(is)) {
      throw runtime_error("reader macro '#': unexpected eof");
    }
    char y = read_char(is);
    switch (y) {
      case '\\': {
        expr_t* expr = read(is, true);
        if (is_eof(expr)) {
          throw runtime_error("reader macro '#': unexpected EOF");
        } else if (is_symbol(expr)) {
          if (expr_t* result = memory.make_char(get_symbol(expr))) {
            return result;
          } else {
            throw expr_exception_t("reader macro '#': undefined mapping from symbol to character", expr);
          }
        } else if (is_integer(expr)) {
          if (expr_t* result = memory.make_char('0' + get_integer(expr))) {
            return result;
          } else {
            throw expr_exception_t("reader macro '#': undefined mapping from integer to character", expr);
          }
        } else {
          throw expr_exception_t("reader macro '#': macro undefined for expr type", expr);
        }
      } break ;
      case '|': {
        int depth = 1;
        while (!is_at_end(is)) {
          if (read_char(is, "|#")) {
            if (--depth == 0) {
              return (expr_t*) 0;
            }
          } else if (read_char(is, "#|")) {
            ++depth;
          }
          read_char(is);
        }
        throw runtime_error("reader macro '#|': unterminated comment");
        return (expr_t*) 0;
      } break ;
      case 't': {
        return memory.make_boolean(true);
      } break ;
      case 'f': {
        return memory.make_boolean(false);
      } break ;
      default: throw runtime_error("reader macro '#': undefined macro for character: '" + string(1, y) + "'");
    }
    assert(0);
    return (expr_t*) 0;
  });

  global_env.define(memory.make_symbol("cin"),  memory.make_istream(cin));
  global_env.define(memory.make_symbol("cout"), memory.make_ostream(cout));
  global_env.define(memory.make_symbol("car"),  memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return car(car(expr)); }, 1, false));
  global_env.define(memory.make_symbol("cdr"),  memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return cdr(car(expr)); }, 1, false));
  global_env.define(memory.make_symbol("cons"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return cons(car(expr), car(cdr(expr))); }, 2, false));
  global_env.define(memory.make_symbol("list"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return expr; }, 0, true));

  global_env.define(memory.make_symbol("eq?"),      memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_eq(car(expr), car(cdr(expr)))); }, 2, false));
  global_env.define(memory.make_symbol("symbol?"),  memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_symbol(car(expr))); }, 1, false));
  global_env.define(memory.make_symbol("string?"),  memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_string(car(expr))); }, 1, false));
  global_env.define(memory.make_symbol("list?"),    memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_list(car(expr))); }, 1, false));
  global_env.define(memory.make_symbol("nil?"),     memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_nil(car(expr))); }, 1, false));
  global_env.define(memory.make_symbol("integer?"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_integer(car(expr))); }, 1, false));
  global_env.define(memory.make_symbol("cons?"),    memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_cons(car(expr))); }, 1, false));

  global_env.define(memory.make_symbol("display"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    expr = car(expr);
    if (is_string(expr)) {
      string s = expr->to_string();
      cout << s.substr(1, s.size() - 2);
    } else {
      cout << expr->to_string();
    }
    return memory.make_void();
  }, 1, false));
  global_env.define(memory.make_symbol("newline"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    cout << endl;
    return memory.make_void();
  }, 0, false));

  global_env.define(memory.make_symbol("+"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_add(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(memory.make_symbol("-"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_sub(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(memory.make_symbol("*"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    expr_t* result = memory.make_real(1.0);
    while (!is_nil(expr)) {
      result = number_mul(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(memory.make_symbol("/"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    if (list_length_internal(expr) == 1) {
      return number_div(memory.make_real(1.0), car(expr));
    }
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_div(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(memory.make_symbol("="), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    expr_t* first = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      if (is_false(number_eq(first, car(expr)))) {
        return memory.make_boolean(false);
      }
      expr = cdr(expr);
    }
    return memory.make_boolean(true);
  }, 1, true));
  global_env.define(memory.make_symbol("<"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    return memory.make_boolean(get_real(list_ref(expr, 0)) < get_real(list_ref(expr, 1)));
  }, 2, false));

  global_env.define(memory.make_symbol("read"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return read(get_istream(list_ref(expr, 0))); }, 1, false));
  global_env.define(memory.make_symbol("eval"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return eval(list_ref(expr, 0), env); }, 1, false));
  global_env.define(memory.make_symbol("apply"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { return apply(list_ref(expr, 0), list_ref(expr, 1), env); }, 2, false));

  global_env.define(memory.make_symbol("begin"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    expr_t* result = memory.make_void();
    while (!is_nil(expr)) {
      result = eval(car(expr), env);
      expr = cdr(expr);
    }
    return result;
  }));
  global_env.define(memory.make_symbol("quote"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    // 'expr
    return car(expr);
  }));

  global_env.define(memory.make_symbol("quasiquote"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    expr = memory.shallow_copy(expr);
    quasiquote(car(expr), env, expr);
    return car(expr);
  }));

  global_env.define(memory.make_symbol("if"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    int len = list_length_internal(expr);
    if (!(len == 2 || len == 3)) {
      throw expr_exception_t("if: wrong arity for special form", expr);
    }
    // (if condition consequence alternative)
    if (is_true(eval(list_ref(expr, 0), env))) {
      return eval(list_ref(expr, 1), env);
    } else if (!is_nil(list_tail(expr, 2))) {
      return eval(list_ref(expr, 2), env);
    } else {
      return memory.make_void();
    }
  }));
  global_env.define(memory.make_symbol("read-char"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    if (&cin == &get_istream(car(expr))) {
      assert(read_char(get_istream(car(expr))) == '\n');
    }
    char c = read_char(get_istream(car(expr)));
    if (c == EOF) {
      return memory.make_eof();
    }
    return memory.make_char(c);
  }, 1, false));
  global_env.define(memory.make_symbol("unread-char"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) { unread_char(get_istream(car(expr)), get_char(list_ref(expr, 1))); return memory.make_void(); }, 2, false));
  global_env.define(memory.make_symbol("peek-char"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    char c = peek_char(get_istream(car(expr)));
    if (c == EOF) {
      return memory.make_eof();
    }
    return memory.make_char(c);
  }, 1, false));
  global_env.define(memory.make_symbol("open-file"), memory.make_primitive_proc([this](expr_t* expr, expr_env_t* env) {
    string filename = get_string(car(expr));
    unique_ptr<ifstream> ifs = make_unique<ifstream>(filename);
    if (!*ifs) {
      throw runtime_error("open-file: failed to open file: '" + filename + "'");
    }
    return memory.make_istream(std::move(ifs));
  }, 1, false));
  //global_env.define(memory.make_symbol("defreader"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
  //  expr_t* symbol = list_ref(expr, 0);
  //  expr_t* body = list_ref(expr, 1);
  //  set_macro_character(get_char(symbol), [this, body](reader_t& reader, const string&, istream& is) {
  //    return eval(body);
  //  });
  //  return body;
  //}));
  global_env.define(memory.make_symbol("defmacro"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    // (defmacro symbol (params) expand-to)
    return env->define(list_ref(expr, 0), memory.make_macro([this, expr](expr_t* args, expr_env_t* env) {
      return begin(list_tail(expr, 2), extend_env(env, list_ref(expr, 1), args));
    }));
  }));
  global_env.define(memory.make_symbol("macroexpand"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    return macroexpand(eval(car(expr)), env);
  }));
  global_env.define(memory.make_symbol("macroexpand-all"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    return macroexpand_all(eval(car(expr)), env);
  }));

  global_env.define(memory.make_symbol("define"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    int len = list_length_internal(expr);
    if (len == -1) {
      throw expr_exception_t("define: cycle in list", expr);
    }
    expr_t* first = list_ref(expr, 0);
    expr_t* rest = cdr(expr);
    if (is_list(first)) {
      // (define (fn args ...) body)
      return env->define(car(first), memory.make_compound_proc(cdr(first), rest));
    } else if (is_symbol(first)) {
      // (define var val)
      if (len != 2) {
        throw expr_exception_t("define: unrecognized special form", expr);
      }
      return env->define(first, eval(car(rest), env));
    } else {
      throw expr_exception_t("unrecognized special form of define", expr);
    }
  }));

  global_env.define(memory.make_symbol("lambda"), memory.make_special_form([this](expr_t* expr, expr_env_t* env) {
    // (lambda (params) body)
    int len = list_length_internal(expr);
    if (len == -1) {
      throw expr_exception_t("lambda: circle in list", expr);
    }
    expr_t* params = car(expr);
    expr_t* body = cdr(expr);
    if (!is_list(params)) {
      throw expr_exception_t("lambda: unrecognized special form", expr);
    }
    return memory.make_compound_proc(params, body);
  }));
}

void interpreter_t::set_macro_character(char c, function<expr_t*(istream&, char)> f) {
  m_reader_macros[(unsigned char)c] = f;
}

function<expr_t*(istream&, char)> interpreter_t::get_macro_character(char c) const {
  function<expr_t*(istream&, char)> result = m_reader_macros[(unsigned char)c];
  if (!result) {
    throw runtime_error("not implemented macro character " + string(1, c));
  }
  return result;
}

expr_t* interpreter_t::read(istream& is, bool recursive) {
  string lexeme;
  return read(lexeme, is, recursive);
}

expr_t* interpreter_t::read(string& lexeme, istream& is, bool recursive) {
  if (is_at_end(is)) {
    return memory.make_eof();
  }

  unsigned char c = (unsigned char) read_char(is);
  if (is_whitespace(c)) {
    return read_whitespace(lexeme, is, c, recursive);
  } else if (is_terminating_macro(c)) {
    return read_terminating_macro(lexeme, is, c, recursive);
  } else if (is_non_terminating_macro(c)) {
    return read_non_terminating_macro(lexeme, is, c, recursive);
  } else if (is_single_escape(c)) {
    return read_single_escape(lexeme, is, c, recursive);
  } else if (is_multiple_escape(c)) {
    return read_multiple_escape(lexeme, is, c, recursive);
  } else if (is_constituent(c)) {
    return read_constituent(lexeme, is, c, recursive);
  } else {
    assert(is_illegal(c));
    return read_illegal();
  }
}

expr_t* interpreter_t::read_illegal() {
  throw runtime_error("illegal character");
}

expr_t* interpreter_t::read_whitespace(string& lexeme, istream& is, char c, bool recursive) {
  if (recursive) {
    lexeme.push_back(c);
  }
  return read(lexeme, is, recursive);
}

expr_t* interpreter_t::read_terminating_macro(string& lexeme, istream& is, char c, bool recursive) {
  expr_t* result = get_macro_character(c)(is, c);
  if (result) {
    return result;
  }
  return read(lexeme, is, recursive);
}

expr_t* interpreter_t::read_non_terminating_macro(string& lexeme, istream& is, char c, bool recursive) {
  expr_t* result = get_macro_character(c)(is, c);
  if (result) {
    return result;
  }
  return read(lexeme, is, recursive);
}

expr_t* interpreter_t::read_single_escape(string& lexeme, istream& is, char c, bool recursive) {
  if (is_at_end(is)) {
    throw runtime_error("single escape: end of file, expect character");
  }
  char y = read_char(is);
  lexeme.clear();
  lexeme.push_back(y);
  return reader_step8(lexeme, is, y, recursive);
}

expr_t* interpreter_t::read_multiple_escape(string& lexeme, istream& is, char c, bool recursive) {
  lexeme.clear();
  return reader_step9(lexeme, is, c, recursive);
}

expr_t* interpreter_t::read_constituent(string& lexeme, istream& is, char c, bool recursive) {
  lexeme.clear();
  lexeme.push_back(c);
  return reader_step8(lexeme, is, c, recursive);
}

expr_t* interpreter_t::reader_step8(string& lexeme, istream& is, char c, bool recursive) {
  if (is_at_end(is)) {
    return reader_step10(lexeme);
  }
  char y = read_char(is);
  if (is_constituent(y) || is_non_terminating_macro(y)) {
    lexeme.push_back(y);
    return reader_step8(lexeme, is, y, recursive);
  } else if (is_single_escape(y)) {
    if (is_at_end(is)) {
      throw runtime_error("expect char");
    }
    char z = read_char(is);
    lexeme.push_back(z);
    return reader_step8(lexeme, is, z, recursive);
  } else if (is_multiple_escape(y)) {
    return reader_step9(lexeme, is, y, recursive);
  } else if (is_terminating_macro(y)) {
    unread_char(is, y);
    return reader_step10(lexeme);
  } else if (is_whitespace(y)) {
    if (!recursive) {
      unread_char(is, y);
    }
    return reader_step10(lexeme);
  } else {
    assert(is_illegal(y));
    throw runtime_error("illegal character");
  }
}

expr_t* interpreter_t::reader_step9(string& lexeme, istream& is, char c, bool recursive) {
  if (is_at_end(is)) {
    throw runtime_error("eof");
  }

  char y = read_char(is);
  if (is_constituent(y) || is_terminating_macro(y) || is_non_terminating_macro(y) || is_whitespace(y)) {
    lexeme.push_back(y);
    return reader_step9(lexeme, is, y, recursive);
  } else if (is_single_escape(y)) {
    if (is_at_end(is)) {
      throw runtime_error("eof");
    }
    char z = read_char(is);
    lexeme.push_back(z);
    return reader_step9(lexeme, is, z, recursive);
  } else if (is_multiple_escape(y)) {
    return reader_step8(lexeme, is, y, recursive);
  } else {
    assert(is_illegal(y));
    throw runtime_error("illegal character");
  }
}

expr_t* interpreter_t::reader_step10(string& lexeme) {
  if ((lexeme[0] == '.' && 1 < lexeme.size()) || ('0' <= lexeme[0] && lexeme[0] <= '9')) {
    bool had_dot = false;
    for (int i = 0; i < lexeme.size(); ++i) {
      if (lexeme[i] == '.') {
        if (!had_dot) {
          had_dot = true;
        } else {
          return memory.make_symbol(lexeme);
        }
      } else if ('0' <= lexeme[i] && lexeme[i] <= '9') {
      } else {
        return memory.make_symbol(lexeme);
      }
    }
    return memory.make_real(stod(lexeme));
  }
  return memory.make_symbol(lexeme);
}

void interpreter_t::print(ostream& os, expr_t* expr) {
  if (!::is_void(expr)) {
    os << expr->to_string() << endl;
  }
}

void interpreter_t::source(const char* filepath) {
  ifstream ifs(filepath);
  if (!ifs) {
    throw runtime_error(string("could not open file '") + filepath + "'");
  }
  source(ifs);
}

void interpreter_t::source(ostream& os, istream& is) {
  while (is) {
    try {
      expr_t* expr = read(is);
      assert(expr);
      print(os, eval(expr));
      if (is_eof(expr)) {
        break ;
      }
    } catch (expr_exception_t& e) {
      os << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " <<  e.expr->to_string() << endl;
    } catch (exception& e) {
      os << "exception: " << e.what() << endl;
    }
  }
}

void interpreter_t::source(istream& is) {
  while (is) {
    try {
      expr_t* expr = read(is);
      assert(expr);
      eval(expr);
      if (is_eof(expr)) {
        break ;
      }
    } catch (expr_exception_t& e) {
      cerr << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " <<  e.expr->to_string() << endl;
    } catch (exception& e) {
      cerr << "exception: " << e.what() << endl;
    }
  }
}

void interpreter_t::repl() {
  while (cin) {
    try {
      expr_t* expr = read(cin);
      assert(expr);
      if (is_eof(expr)) {
        break ;
      }
      //assert(read_char(cin) == '\n');
      print(cout, eval(expr));
    } catch (expr_exception_t& e) {
      cout << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " <<  e.expr->to_string() << endl;
    } catch (exception& e) {
      cout << "exception: " << e.what() << endl;
    }
  }
}

expr_t* interpreter_t::eval(expr_t* expr) {
  return eval(expr, &global_env);
}

expr_t* interpreter_t::eval(expr_t* expr, expr_env_t* env) {
  switch (expr->type) {
  case expr_type_t::END_OF_FILE:
  case expr_type_t::VOID:
  case expr_type_t::NIL:
  case expr_type_t::BOOLEAN:
  case expr_type_t::CHAR:
  case expr_type_t::INTEGER:
  case expr_type_t::REAL:
  case expr_type_t::STRING: return expr;
  case expr_type_t::SYMBOL: {
    if (expr_t* result = env->get(expr)) {
      return result;
    }
    throw expr_exception_t("eval: symbol not defined", expr);
  } break ;
  case expr_type_t::CONS: {
    expr_t* the_operator = eval(car(expr), env);
    expr_t* the_operands = cdr(expr);
    if (is_special_form(the_operator)) {
      return apply_special_form(the_operator, the_operands, env);
    } else if (is_macro(the_operator)) {
      return apply_macro(the_operator, the_operands, env);
    } else {
      return apply(the_operator, list_map(the_operands, [this, env](expr_t* expr) { return eval(expr, env); }), env);
    }
  } break ;
  case expr_type_t::MACRO:
  case expr_type_t::ENV:
  case expr_type_t::PRIMITIVE_PROC: 
  case expr_type_t::SPECIAL_FORM:
  case expr_type_t::COMPOUND_PROC: throw expr_exception_t("eval: unexpected type", expr);
  default: assert(0);
  }
}

expr_t* interpreter_t::list_of_values(expr_t* arguments, expr_t* parameters, expr_env_t* env) {
  expr_t* result = memory.make_nil();
  while (!is_nil(parameters) && !is_nil(arguments)) {
    if (is_eq(car(parameters), dot())) {
      expr_t* rest = memory.make_nil();
      while (!is_nil(arguments)) {
        rest = list_append(rest, cons(eval(car(arguments), env), memory.make_nil()), false);
        arguments = cdr(arguments);
      }
      result = list_append(result, cons(rest, memory.make_nil()), false);
      break ;
    }
    result = list_append(result, cons(eval(car(arguments), env), memory.make_nil()), false);
    parameters = cdr(parameters);
    arguments = cdr(arguments);
  }
  return result;
}

expr_t* interpreter_t::begin(expr_t* expr, expr_env_t* env) {
  expr_t* result = memory.make_void();
  while (is_cons(expr)) {
    result = eval(car(expr), env);
    expr = cdr(expr);
  }
  if (!is_nil(expr)) {
    result = eval(expr, env);
  }
  return result;
}

expr_t* interpreter_t::apply(expr_t* expr, expr_t* args, expr_env_t* env) {
  if (is_primitive_proc(expr)) {
    return apply_primitive_proc(expr, args, env);
  } else if (is_compound_proc(expr)) {
    return apply_compound_proc(expr, args, env);
  } else {
    throw expr_exception_t("unexpected type in apply", expr);
  }
}

expr_t* interpreter_t::apply_special_form(expr_t* expr, expr_t* args, expr_env_t* env) {
  assert(expr->type == expr_type_t::SPECIAL_FORM);
  return ((expr_special_form_t*)expr)->f(args, env);
}

expr_t* interpreter_t::apply_primitive_proc(expr_t* expr, expr_t* args, expr_env_t* env) {
  int len = list_length_internal(args);
  if (len == -1) {
    throw expr_exception_t("argument list has cycles for primitive procedure", args);
  }
  expr_primitive_proc_t* proc = (expr_primitive_proc_t*)expr;
  if (proc->is_variadic && len < proc->arity || !proc->is_variadic && len != proc->arity) {
    throw expr_exception_t("argument list arity is not defined for primitive procedure", args);
  }
  // is variadic.. if so, find variadic expr, then bind all remaining parameters to the last argument
  return proc->f(args, env);
}

expr_t* interpreter_t::apply_compound_proc(expr_t* expr, expr_t* args, expr_env_t* env) {
  expr_t* body = get_compound_proc_body(expr);
  expr_t* params = get_compound_proc_params(expr);
  return begin(body, extend_env(env, params, args));
}

expr_t* interpreter_t::apply_macro(expr_t* expr, expr_t* args, expr_env_t* env) {
  expr_macro_t* macro = (expr_macro_t*)expr;
  return eval(macro->f(args, env), env);
}

expr_t* interpreter_t::macroexpand_all(expr_t* expr, expr_env_t* env) {
  expr = macroexpand(expr, env);
  if (is_list(expr)) {
    return list_map(expr, [this, env](expr_t* expr) {
      return macroexpand_all(expr, env);
    });
  }
  return expr;
}

expr_t* interpreter_t::macroexpand(expr_t* expr, expr_env_t* env) {
  while (1) {
    expr_t* next = expand(expr, env);
    if (next == expr) {
      break ;
    }
    expr = next;
  }
  return expr;
}

expr_t* interpreter_t::expand(expr_t* expr, expr_env_t* env) {
  if (!is_list(expr)) {
    return expr;
  }
  expr_t* head = car(expr);
  if (is_symbol(head)) {
    expr_t* binding = env->get(head);
    if (!binding) {
      throw expr_exception_t("expand: symbol not defined", head);
    }
    if (is_macro(binding)) {
      expr_macro_t* macro = (expr_macro_t*)binding;
      return macro->f(cdr(expr), env);
    }
  }
  return expr;
}

void interpreter_t::quasiquote(expr_t* expr, expr_env_t* env, expr_t* car_parent) {
  if (!is_cons(expr)) {
    return ;
  }

  if (is_unquote(car(expr))) {
    if (!car_parent) {
      throw expr_exception_t("unquote: can only unquote inside of a quasiquote context", expr);
    }
    set_car(car_parent, eval(list_ref(expr, 1), env));
    return ;
  }

  if (is_unquote_splicing(car(expr))) {
    if (!car_parent) {
      throw expr_exception_t("unquote-splicing: can only unquote inside of a quasiquote context", expr);
    }
    expr = eval(list_ref(expr, 1), env);
    if (!is_cons(expr)) {
      if (is_nil(expr)) {
        expr = memory.make_void();
      }
      set_car(car_parent, expr);
      return ;
    }
    while (is_cons(expr)) {
      set_car(car_parent, car(expr));
      if (is_cons(cdr(expr))) {
        set_cdr(car_parent, cons(0, cdr(car_parent)));
        car_parent = cdr(car_parent);
      }
      expr = cdr(expr);
    }
    if (!is_nil(expr)) {
      set_cdr(car_parent, cons(expr, cdr(car_parent)));
    }
    return ;
  }

  while (is_cons(expr)) {
    quasiquote(car(expr), env, expr);
    expr = cdr(expr);
  }
}

bool interpreter_t::is_unquote(expr_t* expr) {
  return is_eq(expr, memory.make_symbol("unquote"));
}

bool interpreter_t::is_unquote_splicing(expr_t* expr) {
  return is_eq(expr, memory.make_symbol("unquote-splicing"));
}

expr_t* interpreter_t::number_add(expr_t* a, expr_t* b) {
  double result = get_real(a) + get_real(b);
  if ((int64_t) result == result) {
    return memory.make_integer((int64_t) result);
  } else {
    return memory.make_real(result);
  }
}

expr_t* interpreter_t::number_sub(expr_t* a, expr_t* b) {
  double result = get_real(a) - get_real(b);
  if ((int64_t) result == result) {
    return memory.make_integer((int64_t) result);
  } else {
    return memory.make_real(result);
  }
}

expr_t* interpreter_t::number_mul(expr_t* a, expr_t* b) {
  double result = get_real(a) * get_real(b);
  if ((int64_t) result == result) {
    return memory.make_integer((int64_t) result);
  } else {
    return memory.make_real(result);
  }
}

expr_t* interpreter_t::number_div(expr_t* a, expr_t* b) {
  double denom = get_real(b);
  if (denom == 0.0) {
    throw expr_exception_t("cannot divide by 0", b);
  }
  double result = get_real(a) / denom;
  if ((int64_t) result == result) {
    return memory.make_integer((int64_t) result);
  } else {
    return memory.make_real(result);
  }
}

expr_t* interpreter_t::number_eq(expr_t* a, expr_t* b) {
  return memory.make_boolean(get_real(a) == get_real(b));
}

expr_env_t* interpreter_t::extend_env(expr_env_t* env, expr_t* symbols, expr_t* exprs) {
  expr_env_t* result = (expr_env_t*) memory.make_env();
  result->parent = env;
  while (!is_nil(symbols) && !is_nil(exprs)) {
    if (is_eq(car(symbols), dot())) {
      symbols = cdr(symbols);
      result->define(car(symbols), exprs);
      symbols = cdr(symbols);
      if (!is_nil(symbols)) {
        throw runtime_error("extend_env: only one symbol can be supplied after dotted parameter");
      }
      return result;
    }
    result->define(car(symbols), car(exprs));
    symbols = cdr(symbols);
    exprs = cdr(exprs);
  }
  if (is_nil(symbols) && is_nil(exprs)) {
    return result;
  } else if (is_nil(symbols) && !is_nil(exprs)) {
    throw expr_exception_t("extend_env: more exprs supplied than symbols", exprs);
  } else if (!is_nil(symbols) && is_nil(exprs)) {
    if (is_eq(car(symbols), dot())) {
      result->define(list_ref(symbols, 1), memory.make_nil());
    } else {
      throw expr_exception_t("extend_env: more symbols supplied than exprs", symbols);
    }
  } else {
    assert(0);
  }
  return result;
}

expr_t* interpreter_t::cons(expr_t* expr1, expr_t* expr2) {
  return memory.make_cons(expr1, expr2);
}

bool interpreter_t::is_eq(expr_t* expr1, expr_t* expr2) {
  return expr1 == expr2;
}

bool interpreter_t::is_true(expr_t* expr) {
  return !is_false(expr);
}

bool interpreter_t::is_false(expr_t* expr) {
  return is_eq(expr, memory.make_boolean(false)) || is_eq(expr, memory.make_nil());
}

expr_t* interpreter_t::make_list(const initializer_list<expr_t*>& exprs) {
  expr_t* result = memory.make_nil();
  for (auto cur = rbegin(exprs); cur != rend(exprs); ++cur) {
    result = cons(*cur, result);
  }
  return result;
}

bool interpreter_t::is_list(expr_t* expr) {
  while (is_cons(expr)) {
    expr = cdr(expr);
  }
  return is_nil(expr);
}

expr_t* interpreter_t::list_tail(expr_t* expr, int n) {
  while (n--) {
    expr = cdr(expr);
  }
  return expr;
}

expr_t* interpreter_t::list_tail(expr_t* expr, expr_t* integer_expr) {
  if (!is_integer(integer_expr)) {
    throw expr_exception_t("list_tail: integer expected", integer_expr);
  }
  return list_tail(expr, get_integer(integer_expr));
}

expr_t* interpreter_t::list_ref(expr_t* expr, int n) {
  return car(list_tail(expr, n));
}

expr_t* interpreter_t::list_ref(expr_t* expr, expr_t* integer_expr) {
  return car(list_tail(expr, integer_expr));
}

int interpreter_t::list_length_internal(expr_t* expr) {
  int result = 0;
  expr_t* turtle = expr;
  expr_t* hare = expr;
  while (!is_nil(hare)) {
    turtle = cdr(turtle);
    hare = cdr(hare);
    ++result;
    if (!is_nil(hare)) {
      hare = cdr(hare);
      ++result;
    } else {
      break ;
    }
    if (hare == turtle) {
      return -1;
    }
  }
  return result;
}

expr_t* interpreter_t::list_length(expr_t* expr) {
  int result = list_length_internal(expr);
  if (result == -1) {
    return memory.make_nil();
  }
  return memory.make_integer(result);
}

expr_t* interpreter_t::list_map(expr_t* expr, const function<expr_t*(expr_t*)>& f) {
  if (is_nil(expr)) {
    return memory.make_nil();
  }
  return cons(f(car(expr)), list_map(cdr(expr), f));
}

expr_t* interpreter_t::list_reverse(expr_t* expr) {
  expr_t* result = memory.make_nil();
  while (!is_nil(expr)) {
    result = cons(car(expr), result);
    expr = cdr(expr);
  }
  return result;
}

expr_t* interpreter_t::list_copy(expr_t* l) {
  expr_t* first = 0;
  expr_t* cur = 0;
  while (!is_nil(l)) {
    if (!first) {
      first = memory.make_cons(car(l), memory.make_nil());
      cur = first;
    } else {
      set_cdr(cur, memory.make_cons(car(l), memory.make_nil()));
      cur = cdr(cur);
    }
    l = cdr(l);
  }
  return first;
}

expr_t* interpreter_t::list_append(expr_t* l, expr_t* e, bool pure) {
  if (pure) {
    l = list_copy(l);
  }
  expr_t* result = l;
  if (is_nil(l)) {
    return e;
  }
  while (!is_nil(cdr(l))) {
    l = cdr(l);
  }
  set_cdr(l, e);
  return result;
}

bool interpreter_t::is_tagged(expr_t* expr, expr_t* tag) {
  return is_cons(expr) && is_eq(car(expr), tag);
}

bool interpreter_t::is_tagged(expr_t* expr, const string& symbol) {
  return is_cons(expr) && is_symbol(car(expr)) && get_symbol(car(expr)) == symbol;
}

expr_t* interpreter_t::dot() {
  return memory.make_symbol(".");
}

