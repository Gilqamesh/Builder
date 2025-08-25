#include "interpreter.h"

#include <x86intrin.h>

bool interpreter_t::is_whitespace(char c) const {
  assert((size_t) c < sizeof(m_whitespaces) / sizeof(m_whitespaces[0]));
  return m_whitespaces[c];
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

interpreter_t::interpreter_t() {
  global_env = memory.make_env();
  global_reader_env = memory.make_env();

  register_symbols();
  register_reader_macros();
  register_macros();
}

expr_t* interpreter_t::read(istream& is) {
  if (is_at_end(is)) {
    return memory.make_eof();
  }

  node_t* node = &m_reader_macro_root;
  function<expr_t*(istream&)> reader_macro;
  string prefix;
  while (node && !is_at_end(is)) {
    char c = read_char(is);
    prefix.push_back(c);
    size_t child_index = (size_t) c;
    if (sizeof(node->children) / sizeof(node->children[0]) <= child_index) {
      throw runtime_error("read: invalid character at the end of lexeme '" + prefix + "': " + to_string(child_index));
    }
    node = node->children[child_index];
    if (node && node->reader_macro) {
      reader_macro = node->reader_macro;
      prefix.clear();
    }
  }

  while (!prefix.empty()) {
    char c = prefix.back();
    prefix.pop_back();
    unread_char(is, c);
  }
 
  expr_t* result = 0;
  if (reader_macro) {
    result = reader_macro(is);
  } else {
    result = default_reader_macro(is);
  }

  if (::is_void(result)) {
    return read(is);
  }

  return result;
}

expr_t* interpreter_t::default_reader_macro(istream& is) {
  string lexeme;
  while (!is_at_end(is)) {
    char c = peek_char(is);
    size_t child_index = (size_t) c;
    if (sizeof(m_reader_macro_root.children) / sizeof(m_reader_macro_root.children[0]) <= child_index) {
      throw runtime_error("defaul_reader: invalid character found: '" + to_string(child_index) + "'");
    }
    if (c == ')' || m_reader_macro_root.children[c]) {
      break ;
    }
    lexeme.push_back(read_char(is));
  }

  if ((lexeme[0] == '.' && 1 < lexeme.size()) || ('0' <= lexeme[0] && lexeme[0] <= '9')) {
    bool had_dot = false;
    for (int i = 0; i < lexeme.size(); ++i) {
      if (lexeme[i] == '.') {
        if (!had_dot) {
          had_dot = true;
        } else {
          return symbol(lexeme);
        }
      } else if ('0' <= lexeme[i] && lexeme[i] <= '9') {
      } else {
        return symbol(lexeme);
      }
    }
    return memory.make_real(stod(lexeme));
  }

  if (lexeme.empty()) {
    return memory.make_void();
  }
  return symbol(lexeme);
}

void interpreter_t::print(ostream& os, expr_t* expr) {
  if (!::is_void(expr)) {
    os << to_string(expr) << endl;
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
      expr_t* read_expr = read(is);
      assert(read_expr);
      if (is_eof(read_expr)) {
        break ;
      }
      expr_t* evaled_expr = eval(read_expr);
      assert(evaled_expr);
      print(os, evaled_expr);
    } catch (expr_exception_t& e) {
      os << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " << to_string(e.expr) << endl;
    } catch (exception& e) {
      os << "exception: " << e.what() << endl;
    }
  }
}

void interpreter_t::source(istream& is) {
  while (is) {
    try {
      expr_t* read_expr = read(is);
      assert(read_expr);
      if (is_eof(read_expr)) {
        break ;
      }
      expr_t* evaled_expr = eval(read_expr);
      assert(evaled_expr);
    } catch (expr_exception_t& e) {
      cerr << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " << to_string(e.expr) << endl;
    } catch (exception& e) {
      cerr << "exception: " << e.what() << endl;
    }
  }
}

void interpreter_t::repl() {
  while (cin) {
    try {
      expr_t* read_expr = read(cin);
      assert(read_expr);
      if (is_eof(read_expr)) {
        break ;
      }
      expr_t* evaled_expr = eval(read_expr);
      assert(evaled_expr);
      print(cout, evaled_expr);
    } catch (expr_t* expr) {
      cout << "thrown expression caught: " << to_string(expr) << endl;
    } catch (expr_exception_t& e) {
      cout << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " << to_string(e.expr) << endl;
    } catch (exception& e) {
      cout << "exception: " << e.what() << endl;
    }
  }
}

void interpreter_t::register_symbols() {
  //env_define(global_env, symbol("cin"),  memory.make_istream(cin));
  //env_define(global_env, symbol("cout"), memory.make_ostream(cout));
  //env_define(global_env, symbol("*global-environment*"), global_env);
}

interpreter_t::node_t::node_t() {
  for (size_t i = 0; i < sizeof(children) / sizeof(children[0]); ++i) {
    children[i] = 0;
  }
}

void interpreter_t::register_reader_macro(const string& prefix, function<expr_t*(istream&)> f) {
  node_t* node = &m_reader_macro_root;
  size_t prefix_index = 0;
  while (prefix_index < prefix.size()) {
    size_t child_index = (size_t) prefix[prefix_index++];
    assert(child_index < sizeof(node->children) / sizeof(node->children[0]));
    if (!node->children[child_index]) {
      node->children[child_index] = new node_t;
    }
    node = node->children[child_index];
  }
  node->reader_macro = f;
}

void interpreter_t::register_reader_macros() {
  for (size_t i = 0; i < sizeof(m_whitespaces) / sizeof(m_whitespaces[0]); ++i) {
    m_whitespaces[i] = false;
  }
  m_whitespaces['\t'] = true;
  m_whitespaces[' '] = true;
  m_whitespaces['\f'] = true;
  m_whitespaces['\r'] = true;
  m_whitespaces['\n'] = true;
  m_whitespaces['\v'] = true;
 
  for (size_t i = 0; i < sizeof(m_whitespaces) / sizeof(m_whitespaces[0]); ++i) {
    if (m_whitespaces[i]) {
      register_reader_macro(string(1, (char) i), [this](istream& is) {
        while (!is_at_end(is) && is_whitespace(peek_char(is))) {
          read_char(is);
        }
        return memory.make_void();
      });
    }
  }
  register_reader_macro("\"", [this](istream& is) {
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
  register_reader_macro("'", [this](istream& is) {
    return make_list({ symbol("quote"), read(is) });
  });
  register_reader_macro("`", [this](istream& is) {
    return make_list({ symbol("quasiquote"), read(is) });
  });
  register_reader_macro(",", [this](istream& is) {
    if (is_at_end(is)) {
      throw runtime_error("reader macro ',': unexpected eof");
    }
    char y = read_char(is);
    switch (y) {
      case '@': {
        return make_list({ symbol("unquote-splicing"), read(is) });
      } break ;
      default: {
        unread_char(is, y);
        return make_list({ symbol("unquote"), read(is) });
      }
    }
  });
  register_reader_macro(")", [this](istream& is) {
    return symbol(")");
  });
  register_reader_macro("(", [this](istream& is) {
    expr_t* result = 0;
    expr_t* prev = 0;
    while (!is_at_end(is)) {
      expr_t* expr = read(is);
      if (is_eq(expr, symbol(")"))) {
        break ;
      }
      expr_t* cur = cons(expr, memory.make_nil());
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
    if (result) {
      return result;
    }
    return memory.make_nil();
  });

  register_reader_macro("#\\", [this](istream& is) {
    expr_t* expr = read(is);
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
  });
  register_reader_macro("|#", [this](istream& is) {
    throw runtime_error("|#: unstarted comment");
    return memory.make_void();
  });
  register_reader_macro("#|", [this](istream& is) {
    size_t depth = 1;
    while (!is_at_end(is)) {
      char c = read_char(is);
      if (c == '|') {
        if (is_at_end(is)) {
          throw runtime_error("reader macro '#|': unterminated comment");
        }
        if (read_char(is) == '#') {
          if (--depth == 0) {
            return memory.make_void();
          }
        }
      } else if (c == '#') {
        c = read_char(is);
        if (c == '|') {
          ++depth;
        } else {
          unread_char(is, c);
        }
      }
    }
    throw runtime_error("reader macro '#|': unterminated comment");
  });
  register_reader_macro("#t", [this](istream& is) {
    return memory.make_boolean(true);
  });
  register_reader_macro("#f", [this](istream& is) {
    return memory.make_boolean(false);
  });
}  

void interpreter_t::register_macros() {
  env_define(global_env, symbol("throw"), macro([this](expr_t* expr, expr_t* call_env) {
    throw expr;
    return nil();
  }));

  //env_define(global_env, symbol("perf"), primitive("perf", [this](expr_t* expr, expr_t* env) {
  //  cout << "total eval: " << m_total_eval_cy / 1000000.0 << "MCy" << endl;
  //  cout << "total apply: " << m_total_apply_cy / 1000000.0 << "MCy" << endl;
  //  return nil();
  //}));

  env_define(global_env, symbol("car"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg = car(expr);
    expr_t* evaled_arg = eval(arg, call_env);
    return car(evaled_arg);
  }));
  env_define(global_env, symbol("cdr"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg = car(expr);
    expr_t* evaled_arg = eval(arg, call_env);
    return cdr(evaled_arg);
  }));
  env_define(global_env, symbol("cons"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg1 = list_ref(expr, 0);
    expr_t* arg2 = list_ref(expr, 1);
    expr_t* evaled_arg1 = eval(arg1, call_env);
    expr_t* evaled_arg2 = eval(arg2, call_env);
    return cons(evaled_arg1, evaled_arg2);
  }));
  //env_define(global_env, symbol("list"), primitive("list", [this](expr_t* expr, expr_t* env) { return expr; }));
  env_define(global_env, symbol("eq?"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg1 = list_ref(expr, 0);
    expr_t* arg2 = list_ref(expr, 1);
    expr_t* evaled_arg1 = eval(arg1, call_env);
    expr_t* evaled_arg2 = eval(arg2, call_env);
    return memory.make_boolean(is_eq(evaled_arg1, evaled_arg2));
  }));
  env_define(global_env, symbol("macro?"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg = list_ref(expr, 0);
    expr_t* evaled_arg = eval(arg, call_env);
    return memory.make_boolean(is_macro(evaled_arg));
  }));
  //env_define(global_env, symbol("equal?"), primitive("equal?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_equal(car(expr), car(cdr(expr)))); }));
  //env_define(global_env, symbol("symbol?"), primitive("symbol?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_symbol(car(expr))); }));
  //env_define(global_env, symbol("string?"), primitive("string?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_string(car(expr))); }));
  //env_define(global_env, symbol("list?"), primitive("list?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_list(car(expr))); }));
  //env_define(global_env, symbol("nil?"), primitive("nil?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_nil(car(expr))); }));
  //env_define(global_env, symbol("integer?"), primitive("integer?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_integer(car(expr))); }));
  env_define(global_env, symbol("cons?"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg = car(expr);
    expr_t* evaled_arg = eval(arg, call_env);
    return memory.make_boolean(is_cons(evaled_arg));
  }));
  env_define(global_env, symbol("display"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg = car(expr);
    expr_t* evaled_arg = eval(arg, call_env);
    if (is_string(evaled_arg)) {
      string s = to_string(evaled_arg);
      cout << s.substr(1, s.size() - 2);
    } else {
      cout << to_string(evaled_arg);
    }
    return nil();
  }));
  env_define(global_env, symbol("newline"), macro([this](expr_t* expr, expr_t* call_env) {
    cout << endl;
    return nil();
  }));

  env_define(global_env, symbol("+"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* result = memory.make_integer(0);
    expr_t* args = expr;
    while (!is_nil(args)) {
      expr_t* arg = car(args);
      expr_t* evaled_arg = eval(arg, call_env);
      result = number_add(result, evaled_arg);
      args = cdr(args);
    }
    return result;
  }));
  ///env_define(global_env, symbol("-"), primitive("-", [this](expr_t* expr, expr_t* env) {
  ///  expr_t* result = car(expr);
  ///  expr = cdr(expr);
  ///  while (!is_nil(expr)) {
  ///    result = number_sub(result, car(expr));
  ///    expr = cdr(expr);
  ///  }
  ///  return result;
  ///}));
  ///env_define(global_env, symbol("*"), primitive("*", [this](expr_t* expr, expr_t* env) {
  ///  expr_t* result = memory.make_real(1.0);
  ///  while (!is_nil(expr)) {
  ///    result = number_mul(result, car(expr));
  ///    expr = cdr(expr);
  ///  }
  ///  return result;
  ///}));
  ///env_define(global_env, symbol("/"), primitive("/", [this](expr_t* expr, expr_t* env) {
  ///  if (list_length(expr) == 1) {
  ///    return number_div(memory.make_real(1.0), car(expr));
  ///  }
  ///  expr_t* result = car(expr);
  ///  expr = cdr(expr);
  ///  while (!is_nil(expr)) {
  ///    result = number_div(result, car(expr));
  ///    expr = cdr(expr);
  ///  }
  ///  return result;
  ///}));
  ///env_define(global_env, symbol("="), primitive("=", [this](expr_t* expr, expr_t* env) {
  ///  expr_t* first = car(expr);
  ///  expr = cdr(expr);
  ///  while (!is_nil(expr)) {
  ///    if (is_false(number_eq(first, car(expr)))) {
  ///      return memory.make_boolean(false);
  ///    }
  ///    expr = cdr(expr);
  ///  }
  ///  return memory.make_boolean(true);
  ///}));
  ///env_define(global_env, symbol("<"), primitive("<", [this](expr_t* expr, expr_t* env) {
  ///  return memory.make_boolean(get_real(list_ref(expr, 0)) < get_real(list_ref(expr, 1)));
  ///}));

  //env_define(global_env, symbol("memory"), primitive("memory", [this](expr_t* expr, expr_t* env) {
  //  function<size_t(node_t*)> reader_macro_memory_collect = [&](node_t* node) {
  //    size_t result = sizeof(*node);
  //    for (size_t i = 0; i < sizeof(node->children) / sizeof(node->children[0]); ++i) {
  //      if (node->children[i]) {
  //        result += reader_macro_memory_collect(node->children[i]);
  //      }
  //    }
  //    return result;
  //  };
  //  cout << "reader macro: " << reader_macro_memory_collect(&m_reader_macro_root) / 1024.0 << " kB" << endl;
  //  return nil();
  //}));
  //env_define(global_env, symbol("read"), primitive("read", [this](expr_t* expr, expr_t* env) { return read(get_istream(list_ref(expr, 0))); }));
  //env_define(global_env, symbol("write"), primitive("write", [this](expr_t* expr, expr_t* env) {
  //  ostream& os = get_ostream(list_ref(expr, 0));
  //  os << to_string(list_ref(expr, 1));
  //  os.flush();
  //  return list_ref(expr, 1);
  //}));
  env_define(global_env, symbol("eval"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* expr_to_eval = list_ref(expr, 0);
    expr_t* evaled_expr_to_eval = eval(expr_to_eval, call_env);
    return eval(evaled_expr_to_eval, call_env);
  }));
  //env_define(global_env, symbol("read-char"), primitive("read-char", [this](expr_t* expr, expr_t* env) {
  //  if (&cin == &get_istream(car(expr))) {
  //    // todo: this was introduced so that (list (read-char cin)) would work, but it doesn't work for cases where read-char is invoked with cin in some deeper context by some procedure:
  //    // (defmacro "hello" (lambda (is) (read-char is)))
  //    // helloa
  //    //assert(read_char(get_istream(car(expr))) == '\n');
  //  }
  //  char c = read_char(get_istream(car(expr)));
  //  if (c == EOF) {
  //    return memory.make_eof();
  //  }
  //  return memory.make_char(c);
  //}));

  //env_define(global_env, symbol("eq-char?"), primitive("eq-char?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(get_char(list_ref(expr, 0)) == get_char(list_ref(expr, 1))); }));
  //env_define(global_env, symbol("is-whitespace?"), primitive("is-whitespace?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_whitespace(get_char(car(expr)))); }));
  //env_define(global_env, symbol("is-at-end?"), primitive("is-at-end?", [this](expr_t* expr, expr_t* env) { return memory.make_boolean(is_at_end(get_istream(car(expr)))); }));
  //env_define(global_env, symbol("unread-char"), primitive("unread-char", [this](expr_t* expr, expr_t* env) { unread_char(get_istream(car(expr)), get_char(list_ref(expr, 1))); return nil(); }));
  //env_define(global_env, symbol("peek-char"), primitive("peek-char", [this](expr_t* expr, expr_t* env) {
  //  char c = peek_char(get_istream(car(expr)));
  //  if (c == EOF) {
  //    return memory.make_eof();
  //  }
  //  return memory.make_char(c);
  //}));
  //env_define(global_env, symbol("open-file"), primitive("open-file", [this](expr_t* expr, expr_t* env) {
  //  string filename = get_string(car(expr));
  //  string mode = get_symbol(list_ref(expr, 1));
  //  if (mode == "read") {
  //    unique_ptr<ifstream> ifs = make_unique<ifstream>(filename);
  //    if (!*ifs) {
  //      throw runtime_error("open-file: failed to open file: '" + filename + "'");
  //    }
  //    return memory.make_istream(std::move(ifs));
  //  } else if (mode == "write") {
  //    unique_ptr<ofstream> ofs = make_unique<ofstream>(filename);
  //    if (!*ofs) {
  //      throw runtime_error("open-file: failed to open file: '" + filename + "'");
  //    }
  //    return memory.make_ostream(std::move(ofs));
  //  } else {
  //    throw expr_exception_t("unsupported mode", list_ref(expr, 1));
  //  }
  //}));
  //env_define(global_env, symbol("gensym"), primitive("gensym", [this](expr_t* expr, expr_t* env) {
  //  return symbol("#:G" + to_string(m_gensym_counter++));
  //}));

  //env_define(global_env, symbol("defreader"), primitive("defreader", [this](expr_t* expr, expr_t* env) {
  //  // (defreader prefix f)
  //  register_reader_macro(get_string(list_ref(expr, 0)), [expr, env, this](istream& is) {
  //    expr_t* result = apply(list_ref(expr, 1), make_list({ memory.make_istream(is) }), env);
  //    return result;
  //  });
  //  return nil();
  //}));
  env_define(global_env, symbol("apply"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* op = list_ref(expr, 0);
    expr_t* args = list_ref(expr, 1);
    expr_t* evaled_op = eval(op, call_env);
    expr_t* evaled_args = eval(args, call_env);
    return apply(evaled_op, evaled_args, call_env);
  }));
  //env_define_special(global_env, symbol("and"), primitive("and", [this](expr_t* expr, expr_t* env) {
  //  while (!is_nil(expr)) {
  //    if (is_false(eval(car(expr), env))) {
  //      return memory.make_boolean(false);
  //    }
  //    expr = cdr(expr);
  //  }
  //  return memory.make_boolean(true);
  //}));
  //env_define_special(global_env, symbol("or"), primitive("or", [this](expr_t* expr, expr_t* env) {
  //  while (!is_nil(expr)) {
  //    if (is_true(eval(car(expr), env))) {
  //      return memory.make_boolean(true);
  //    }
  //    expr = cdr(expr);
  //  }
  //  return memory.make_boolean(false);
  //}));
  //env_define_special(global_env, symbol("not"), primitive("not", [this](expr_t* expr, expr_t* env) {
  //  return memory.make_boolean(!is_true(eval(car(expr), env)));
  //}));
  // env_define_special(global_env, symbol("begin"), primitive("begin", [this](expr_t* expr, expr_t* env) {
  //  expr_t* result = memory.make_nil();
  //  while (!is_nil(expr)) {
  //    result = eval(car(expr), env);
  //    expr = cdr(expr);
  //  }
  //  return result;
  // }));
  // env_define_special(global_env, symbol("quote"), primitive("quote", [this](expr_t* expr, expr_t* env) {
  //   // 'expr
  //   return car(expr);
  // }));

  //env_define_special(global_env, symbol("quasiquote"), primitive("quasiquote", [this](expr_t* expr, expr_t* env) {
  //  expr = memory.shallow_copy(expr);
  //  quasiquote(car(expr), env, expr);
  //  return car(expr);
  //}));

  //env_define_special(global_env, symbol("set!"), primitive("set!", [this](expr_t* expr, expr_t* env) {
  //  // (set! symbol expr)
  //  return env_set(env, list_ref(expr, 0), eval(list_ref(expr, 1), env));
  //}));
  //env_define(global_env, symbol("set-car!"), primitive("set-car!", [this](expr_t* expr, expr_t* env) {
  //  // (set-car! list new-car)
  //  set_car(list_ref(expr, 0), list_ref(expr, 1));
  //  return nil();
  //}));
  //env_define(global_env, symbol("set-cdr!"), primitive("set-cdr!", [this](expr_t* expr, expr_t* env) {
  //  // (set-cdr! list new-cdr)
  //  set_cdr(list_ref(expr, 0), list_ref(expr, 1));
  //  return nil();
  //}));
  //env_define_special(global_env, symbol("macroexpand"), primitive("macroexpand", [this](expr_t* expr, expr_t* env) {
  //  return macroexpand(eval(car(expr)), env);
  //}));
  //env_define_special(global_env, symbol("macroexpand-all"), primitive("macroexpand-all", [this](expr_t* expr, expr_t* env) {
  //  return macroexpand_all(eval(car(expr)), env);
  //}));

  //env_define(global_env, symbol("quasiquote"), macro([this](expr_t* expr, expr_env_t* env) {
  //  expr_t* extended_env = memory.make_env();
  //  set_env_parent(extended_env, env);

  //  env_define(extended_env, symbol("unquote"), macro([this](expr_t* expr, expr_env_t* env) {
  //  }));
  //  env_define(extended_env, symbol("unquote-splicing"), macro([this](expr_t* expr, expr_env_t* env) {
  //  }));
  //}));

  env_define(global_env, symbol("macroexpand"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* arg = list_ref(expr, 0);
    expr_t* evaled_arg = eval(arg, call_env);
    return evaled_arg;
  }));
  env_define(global_env, symbol("if"), macro([this](expr_t* expr, expr_t* call_env) {
    expr_t* condition = list_ref(expr, 0);
    expr_t* evaled_condition = eval(condition, call_env);
    expr_t* consequence = list_ref(expr, 1);
    if (is_true(evaled_condition)) {
      expr_t* evaled_consequence = eval(consequence, call_env);
      return evaled_consequence;
    } else {
      expr_t* tail = list_tail(expr, 2);
      if (is_cons(tail)) {
        expr_t* alternative = car(tail);
        expr_t* evaled_alternative = eval(alternative, call_env);
        return evaled_alternative;
      }
      return nil();
    }
  }));
  env_define(global_env, symbol("compound"), macro([this](expr_t* expr, expr_t* call_env) {
    // (compound (params) body ...)
    expr_t* params = list_ref(expr, 0);
    expr_t* body = list_ref(expr, 1);
    expr_t* closure_env = list_ref(expr, 2);
    assert(closure_env);
    return macro([this, body, closure_env, params](expr_t* expr, expr_t* call_env) {
      expr_t* args = expr;
      expr_t* evaluated_args = list_of_values(args, call_env);
      expr_t* extended_env = extend_env(closure_env, params, evaluated_args);
      return begin(body, extended_env);
    });
  }));
  env_define(global_env, symbol("define"), macro([this](expr_t* expr, expr_t* call_env) {
    // (define name value)
    expr_t* name = list_ref(expr, 0);
    expr_t* value = list_ref(expr, 1);
    expr_t* evaled_value = eval(value, call_env);
    return env_define(call_env, name, evaled_value);
  }));
  env_define(global_env, symbol("lambda"), macro([this](expr_t* expr, expr_t* call_env) {
    // (lambda (params) body ...)
    return make_list({ symbol("compound"), list_ref(expr, 0), list_tail(expr, 1), call_env });
  }));
  env_define(global_env, symbol("macro"), macro([this](expr_t* expr, expr_t* call_env) {
    // (macro (params) body ...)
    expr_t* params = list_ref(expr, 0);
    expr_t* body = list_tail(expr, 1);
    expr_t* capture_env = call_env;
    return macro([this, body, params, capture_env](expr_t* args, expr_t* call_env) {
      expr_t* extended_env = extend_env(capture_env, params, args);
      return begin(body, extended_env);
    });
  }));
  env_define(global_env, symbol("call-env"), macro([this](expr_t* expr, expr_t* call_env) {
    return call_env;
  }));
}

expr_t* interpreter_t::eval(expr_t* expr) {
  size_t t_start = __rdtsc();
  expr_t* result = eval(expr, global_env);
  m_total_eval_cy += __rdtsc() - t_start;
  return result;
}

expr_t* interpreter_t::eval(expr_t* expr, expr_t* env) {
  expr_t* result = 0;

  if (is_symbol(expr)) {
    result = env_get(env, expr);
    if (!result) {
      throw expr_exception_t("eval: symbol not defined", expr);
    }
  } else if (is_cons(expr)) {
    while (is_cons(expr)) {
    }
  }

  switch (expr->type) {
  case expr_type_t::SYMBOL: {
    result = env_get(env, expr);
    if (!result) {
      throw expr_exception_t("eval: symbol not defined", expr);
    }
  } break ;
  case expr_type_t::CONS: {
    expr_t* the_operator = eval(car(expr), env);
    while (is_cons(the_operator)) {
      the_operator = eval(the_operator, env);
    }
    expr_t* the_operands = cdr(expr);
    result = apply(the_operator, the_operands, env);
  } break ;
  default: {
    result = expr;
  } break ;
  }

  return result;
}

expr_t* interpreter_t::list_of_values(expr_t* args, expr_t* env) {
  if (is_nil(args)) {
    return nil();
  } else {
    return cons(eval(car(args), env), list_of_values(cdr(args), env));
  }
}

expr_t* interpreter_t::begin(expr_t* expr, expr_t* env) {
  expr_t* result = memory.make_nil();
  while (!is_nil(expr)) {
    result = eval(car(expr), env);
    expr = cdr(expr);
  }
  return result;
}

expr_t* interpreter_t::apply(expr_t* expr, expr_t* args, expr_t* env) {
  size_t t_start = __rdtsc();
  expr_t* result = 0;
  if (is_macro(expr)) {
    expr_t* cur_args = args;
    expr_t* prev_args = args;
    while (!is_nil(cur_args)) {
      expr_t* arg = car(cur_args);
      if (is_cons(arg) && is_eq(car(arg), symbol("unquote"))) {
        expr_t* arg_to_evaluate = car(cdr(arg));
        expr_t* evaluated_arg = eval(arg_to_evaluate, env);
        set_car(cur_args, evaluated_arg);
        if (prev_args != cur_args) {
          set_cdr(prev_args, cur_args);
        }
      }
      prev_args = cur_args;
      cur_args = cdr(cur_args);
    }
    result = macro_f(expr)(args, env);
  } else {
    throw expr_exception_t("unexpected type in apply", expr);
  }
  m_total_apply_cy += __rdtsc() - t_start;
  return result;
}

expr_t* interpreter_t::macroexpand_all(expr_t* expr, expr_t* env) {
  expr = macroexpand(expr, env);
  if (is_list(expr)) {
    return list_map(expr, [this, env](expr_t* expr) {
      return macroexpand_all(expr, env);
    });
  }
  return expr;
}

expr_t* interpreter_t::macroexpand(expr_t* expr, expr_t* env) {
  while (1) {
    expr_t* next = expand(expr, env);
    if (next == expr) {
      break ;
    }
    expr = next;
  }
  return expr;
}

expr_t* interpreter_t::expand(expr_t* expr, expr_t* env) {
  if (!is_list(expr)) {
    return expr;
  }
  expr_t* head = car(expr);
  if (is_symbol(head)) {
    if (expr_t* binding = env_get(env, head)) {
      if (is_tagged(binding, "macro")) {
        return eval(expr, env);
      }
    }
  }
  return expr;
}

void interpreter_t::quasiquote(expr_t* expr, expr_t* env, expr_t* car_parent) {
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
  return is_eq(expr, symbol("unquote"));
}

bool interpreter_t::is_unquote_splicing(expr_t* expr) {
  return is_eq(expr, symbol("unquote-splicing"));
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

expr_t* interpreter_t::extend_env(expr_t* parent_env, expr_t* params, expr_t* args) {
  expr_t* result = memory.make_env();
  env_set_parent(result, parent_env);
  while (!is_nil(params) && !is_nil(args)) {
    if (is_eq(car(params), dot())) {
      params = cdr(params);
      env_define(result, car(params), args);
      return result;
    }
    env_define(result, car(params), car(args));
    params = cdr(params);
    args = cdr(args);
  }
  if (is_nil(params) && is_nil(args)) {
  } else if (is_nil(params) && !is_nil(args)) {
    throw expr_exception_t("extend_env: more args supplied than params", args);
  } else if (!is_nil(params) && is_nil(args)) {
    if (is_eq(car(params), dot())) {
      env_define(result, list_ref(params, 1), memory.make_nil());
    } else {
      throw expr_exception_t("extend_env: more params supplied than args", params);
    }
  } else {
    assert(0);
  }
  return result;
}

expr_t* interpreter_t::cons(expr_t* expr1, expr_t* expr2) {
  assert(!::is_void(expr1) && !::is_void(expr2));
  return memory.make_cons(expr1, expr2);
}

expr_t* interpreter_t::nil() {
  return memory.make_nil();
}

expr_t* interpreter_t::symbol(const string& s, bool is_interned) {
  return memory.make_symbol(s, is_interned);
}

expr_t* interpreter_t::macro(function<expr_t*(expr_t* args, expr_t* call_env)> f) {
  return memory.make_macro(f);
}

bool interpreter_t::is_eq(expr_t* expr1, expr_t* expr2) {
  return expr1 == expr2;
}

bool interpreter_t::is_equal(expr_t* expr1, expr_t* expr2) {
  unordered_map<expr_t*, size_t> seen;
  size_t id;
  return is_equal(expr1, expr2, seen, id);
}

bool interpreter_t::is_equal(expr_t* expr1, expr_t* expr2, unordered_map<expr_t*, size_t>& seen, size_t& id) {
  if (expr1->type != expr2->type) {
    return false;
  }
  auto it1 = seen.find(expr1);
  auto it2 = seen.find(expr2);
  if (it1 == seen.end() && it2 == seen.end()) {
    seen[expr1] = id;
    seen[expr2] = id;
    ++id;
  } else if (it1 == seen.end()) {
    return false;
  } else if (it2 == seen.end()) {
    return false;
  } else {
    return it1->second == it2->second;
  }

  switch (expr1->type) {
  case expr_type_t::END_OF_FILE: {
    return true;
  } break ;
  case expr_type_t::NIL: {
    return true;
  } break ;
  case expr_type_t::BOOLEAN: {
    return get_boolean(expr1) == get_boolean(expr2);
  } break ;
  case expr_type_t::CHAR: {
    return get_char(expr1) == get_char(expr2);
  } break ;
  case expr_type_t::INTEGER: {
    return get_integer(expr1) == get_integer(expr2);
  } break ;
  case expr_type_t::REAL: {
    return get_real(expr1) == get_real(expr2);
  } break ;
  case expr_type_t::STRING: {
    return get_string(expr1) == get_string(expr2);
  } break ;
  case expr_type_t::SYMBOL: {
    return get_symbol(expr1) == get_symbol(expr2);
  } break ;
  case expr_type_t::ENV: {
    const auto& bindings1 = get_env_bindings(expr1);
    const auto& bindings2 = get_env_bindings(expr2);
    if (bindings1.size() != bindings2.size()) {
      return false;
    }
    auto it1 = bindings1.begin();
    auto it2 = bindings2.begin();
    while (it1 != bindings1.end() && it2 != bindings2.end()) {
      if (!is_equal(it1->first, it2->first, seen, id) || !is_equal(it1->second, it2->second, seen,id)) {
        return false;
      }
      ++it1;
      ++it2;
    }
    assert(it1 == bindings1.end() && it2 == bindings2.end());
    return true;
  } break ;
  case expr_type_t::CONS: {
    return is_equal(car(expr1), car(expr2), seen, id) && is_equal(cdr(expr1), cdr(expr2), seen, id);
  } break ;
  case expr_type_t::ISTREAM: {
    return &get_istream(expr1) == &get_istream(expr2);
  } break ;
  case expr_type_t::OSTREAM: {
    return &get_ostream(expr1) == &get_ostream(expr2);
  } break ;
  default: assert(0);
  }
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
    if (!::is_void(*cur)) {
      result = cons(*cur, result);
    }
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

size_t interpreter_t::list_length(expr_t* expr) {
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
      throw expr_exception_t("list_length: list is circular", expr);
    }
  }
  return result;
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
  return symbol(".");
}

