#include "interpreter.h"

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
  register_symbols();
  register_reader_macros();
  register_primitive_procs();
  register_special_forms();
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
      expr = compile(expr);
      if (!expr) {
        continue ;
      } 
      if (is_eof(expr)) {
        break ;
      }
      expr = eval(expr);
      expr = compile(expr);
      if (!expr) {
        continue ;
      }
      print(cout, expr);
    } catch (expr_exception_t& e) {
      cout << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " <<  e.expr->to_string() << endl;
    } catch (exception& e) {
      cout << "exception: " << e.what() << endl;
    }
  }
}

void interpreter_t::register_symbols() {
  global_env.define(memory.make_symbol("cin"),  memory.make_istream(cin));
  global_env.define(memory.make_symbol("cout"), memory.make_ostream(cout));
  global_env.define(memory.make_symbol("*global-environment*"), (expr_t*) &global_env);
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
    return make_list({ memory.make_symbol("quote"), read(is) });
  });
  register_reader_macro("`", [this](istream& is) {
    return make_list({ memory.make_symbol("quasiquote"), read(is) });
  });
  register_reader_macro(",", [this](istream& is) {
    if (is_at_end(is)) {
      throw runtime_error("reader macro ',': unexpected eof");
    }
    char y = read_char(is);
    switch (y) {
      case '@': {
        return make_list({ memory.make_symbol("unquote-splicing"), read(is) });
      } break ;
      default: {
        unread_char(is, y);
        return make_list({ memory.make_symbol("unquote"), read(is) });
      }
    }
  });
  register_reader_macro(")", [this](istream& is) {
    return memory.make_symbol(")");
  });
  register_reader_macro("(", [this](istream& is) {
    expr_t* result = 0;
    expr_t* prev = 0;
    while (!is_at_end(is)) {
      expr_t* expr = read(is);
      if (is_eq(expr, memory.make_symbol(")"))) {
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
  register_reader_macro("#|", [this](istream& is) {
    int depth = 1;
    while (!is_at_end(is)) {
      if (read_char(is, "|#")) {
        if (--depth == 0) {
          return memory.make_void();
        }
      } else if (read_char(is, "#|")) {
        ++depth;
      }
      read_char(is);
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

expr_t* interpreter_t::register_macro(expr_env_t* env, const string& name, function<expr_t*(expr_t*, expr_env_t*)> f) {
  return global_env.define(memory.make_symbol(name), memory.make_macro(name, f));
}

void interpreter_t::register_macros() {
}

expr_t* interpreter_t::register_primitive_proc(expr_env_t* env, const string& name, function<expr_t*(expr_t*, expr_env_t*)> f) {
  return global_env.define(memory.make_symbol(name), memory.make_primitive_proc(name, f));
}

void interpreter_t::register_primitive_procs() {
  register_primitive_proc(&global_env, "car",      [this](expr_t* expr, expr_env_t* env) { return car(car(expr)); });
  register_primitive_proc(&global_env, "cdr",      [this](expr_t* expr, expr_env_t* env) { return cdr(car(expr)); });
  register_primitive_proc(&global_env, "cons",     [this](expr_t* expr, expr_env_t* env) { return cons(car(expr), car(cdr(expr))); });
  register_primitive_proc(&global_env, "list",     [this](expr_t* expr, expr_env_t* env) { return expr; });
  register_primitive_proc(&global_env, "eq?",      [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_eq(car(expr), car(cdr(expr)))); });
  register_primitive_proc(&global_env, "symbol?",  [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_symbol(car(expr))); });
  register_primitive_proc(&global_env, "string?",  [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_string(car(expr))); });
  register_primitive_proc(&global_env, "list?",    [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_list(car(expr))); });
  register_primitive_proc(&global_env, "nil?",     [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_nil(car(expr))); });
  register_primitive_proc(&global_env, "integer?", [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_integer(car(expr))); });
  register_primitive_proc(&global_env, "cons?",    [this](expr_t* expr, expr_env_t* env) { return memory.make_boolean(is_cons(car(expr))); });
  register_primitive_proc(&global_env, "display",  [this](expr_t* expr, expr_env_t* env) {
    expr = car(expr);
    if (is_string(expr)) {
      string s = expr->to_string();
      cout << s.substr(1, s.size() - 2);
    } else {
      cout << expr->to_string();
    }
    return memory.make_void();
  });
  register_primitive_proc(&global_env, "newline", [this](expr_t* expr, expr_env_t* env) {
    cout << endl;
    return memory.make_void();
  });

  register_primitive_proc(&global_env, "+", [this](expr_t* expr, expr_env_t* env) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_add(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  });
  register_primitive_proc(&global_env, "-", [this](expr_t* expr, expr_env_t* env) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_sub(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  });
  register_primitive_proc(&global_env, "*", [this](expr_t* expr, expr_env_t* env) {
    expr_t* result = memory.make_real(1.0);
    while (!is_nil(expr)) {
      result = number_mul(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  });
  register_primitive_proc(&global_env, "/", [this](expr_t* expr, expr_env_t* env) {
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
  });
  register_primitive_proc(&global_env, "=", [this](expr_t* expr, expr_env_t* env) {
    expr_t* first = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      if (is_false(number_eq(first, car(expr)))) {
        return memory.make_boolean(false);
      }
      expr = cdr(expr);
    }
    return memory.make_boolean(true);
  });
  register_primitive_proc(&global_env, "<", [this](expr_t* expr, expr_env_t* env) {
    return memory.make_boolean(get_real(list_ref(expr, 0)) < get_real(list_ref(expr, 1)));
  });

  register_primitive_proc(&global_env, "memory", [this](expr_t* expr, expr_env_t* env) {
    function<size_t(node_t*)> reader_macro_memory_collect = [&](node_t* node) {
      size_t result = sizeof(*node);
      for (size_t i = 0; i < sizeof(node->children) / sizeof(node->children[0]); ++i) {
        if (node->children[i]) {
          result += reader_macro_memory_collect(node->children[i]);
        }
      }
      return result;
    };
    cout << "reader macro: " << reader_macro_memory_collect(&m_reader_macro_root) / 1024.0 << " kB" << endl;
    return memory.make_void();
  });
  register_primitive_proc(&global_env, "read", [this](expr_t* expr, expr_env_t* env) { return read(get_istream(list_ref(expr, 0))); });
  register_primitive_proc(&global_env, "write", [this](expr_t* expr, expr_env_t* env) {
    ostream& os = get_ostream(list_ref(expr, 0));
    os << list_ref(expr, 1)->to_string();
    os.flush();
    return list_ref(expr, 1);
  });
  register_primitive_proc(&global_env, "eval", [this](expr_t* expr, expr_env_t* env) { return eval(list_ref(expr, 0), env); });
  register_primitive_proc(&global_env, "apply", [this](expr_t* expr, expr_env_t* env) { return apply(list_ref(expr, 0), list_ref(expr, 1), env); });
  register_primitive_proc(&global_env, "read-char", [this](expr_t* expr, expr_env_t* env) {
    if (&cin == &get_istream(car(expr))) {
      // todo: this was introduced so that (list (read-char cin)) would work, but it doesn't work for cases where read-char is invoked with cin in some deeper context by some procedure:
      // (defmacro "hello" (lambda (is) (read-char is)))
      // helloa
      //assert(read_char(get_istream(car(expr))) == '\n');
    }
    char c = read_char(get_istream(car(expr)));
    if (c == EOF) {
      return memory.make_eof();
    }
    return memory.make_char(c);
  });
  register_primitive_proc(&global_env, "unread-char", [this](expr_t* expr, expr_env_t* env) { unread_char(get_istream(car(expr)), get_char(list_ref(expr, 1))); return memory.make_void(); });
  register_primitive_proc(&global_env, "peek-char", [this](expr_t* expr, expr_env_t* env) {
    char c = peek_char(get_istream(car(expr)));
    if (c == EOF) {
      return memory.make_eof();
    }
    return memory.make_char(c);
  });
  register_primitive_proc(&global_env, "open-file", [this](expr_t* expr, expr_env_t* env) {
    string filename = get_string(car(expr));
    string mode = get_symbol(list_ref(expr, 1));
    if (mode == "read") {
      unique_ptr<ifstream> ifs = make_unique<ifstream>(filename);
      if (!*ifs) {
        throw runtime_error("open-file: failed to open file: '" + filename + "'");
      }
      return memory.make_istream(std::move(ifs));
    } else if (mode == "write") {
      unique_ptr<ofstream> ofs = make_unique<ofstream>(filename);
      if (!*ofs) {
        throw runtime_error("open-file: failed to open file: '" + filename + "'");
      }
      return memory.make_ostream(std::move(ofs));
    } else {
      throw expr_exception_t("unsupported mode", list_ref(expr, 1));
    }
  });
  register_primitive_proc(&global_env, "defreader", [this](expr_t* expr, expr_env_t* env) {
    // (defreader prefix f)
    register_reader_macro(get_string(list_ref(expr, 0)), [expr, env, this](istream& is) {
      return apply(eval(list_ref(expr, 1)), make_list({ memory.make_istream(is) }), env);
    });
    return memory.make_void();
  });
}

expr_t* interpreter_t::register_special_form(expr_env_t* env, const string& name, function<expr_t*(expr_t*, expr_env_t*)> f) {
  return global_env.define(memory.make_symbol(name), memory.make_special_form(name, f));
}

void interpreter_t::register_special_forms() {
  register_special_form(&global_env, "begin", [this](expr_t* expr, expr_env_t* env) {
    expr_t* result = memory.make_void();
    while (!is_nil(expr)) {
      result = eval(car(expr), env);
      expr = cdr(expr);
    }
    return result;
  });
  register_special_form(&global_env, "quote", [this](expr_t* expr, expr_env_t* env) {
    // 'expr
    return car(expr);
  });

  register_special_form(&global_env, "quasiquote", [this](expr_t* expr, expr_env_t* env) {
    expr = memory.shallow_copy(expr);
    quasiquote(car(expr), env, expr);
    return car(expr);
  });

  register_special_form(&global_env, "if", [this](expr_t* expr, expr_env_t* env) {
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
  });
  register_special_form(&global_env, "defmacro", [this](expr_t* expr, expr_env_t* env) {
    // (defmacro symbol (params) expand-to)
    return register_macro(env, get_symbol(list_ref(expr, 0)), [this, expr](expr_t* args, expr_env_t* env) {
      return begin(list_tail(expr, 2), extend_env(env, list_ref(expr, 1), args));
    });
  });
  register_special_form(&global_env, "macroexpand", [this](expr_t* expr, expr_env_t* env) {
    return macroexpand(eval(car(expr)), env);
  });
  register_special_form(&global_env, "macroexpand-all", [this](expr_t* expr, expr_env_t* env) {
    return macroexpand_all(eval(car(expr)), env);
  });

  register_special_form(&global_env, "define", [this](expr_t* expr, expr_env_t* env) {
    size_t len = list_length_internal(expr);
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
  });

  register_special_form(&global_env, "lambda", [this](expr_t* expr, expr_env_t* env) {
    // (lambda (params) body)
    size_t len = list_length_internal(expr);
    expr_t* params = car(expr);
    expr_t* body = cdr(expr);
    if (!is_list(params)) {
      throw expr_exception_t("lambda: unrecognized special form", expr);
    }
    return memory.make_compound_proc(params, body);
  });
}

expr_t* interpreter_t::eval(expr_t* expr) {
  return eval(expr, &global_env);
}

expr_t* interpreter_t::eval(expr_t* expr, expr_env_t* env) {
  switch (expr->type) {
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
  default: return expr;
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
  expr_primitive_proc_t* proc = (expr_primitive_proc_t*)expr;
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
    if (expr_t* binding = env->get(head)) {
      if (is_macro(binding)) {
        expr_macro_t* macro = (expr_macro_t*)binding;
        return macro->f(cdr(expr), env);
      }
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

expr_t* interpreter_t::compile(expr_t* expr) {
  expr = compile_void_exprs_out(expr);
  if (!expr) {
    return 0;
  }
  // do other compile stuff
  return expr;
}

expr_t* interpreter_t::compile_void_exprs_out(expr_t* expr) {
  if (is_cons(expr)) {
    expr_t* new_car = compile_void_exprs_out(car(expr));
    expr_t* new_cdr = compile_void_exprs_out(cdr(expr));
    if (!new_car && !new_cdr) {
      return 0;
    } else if (!new_car) {
      return new_cdr;
    } else if (!new_cdr) {
      return new_car;
    } else {
      set_car(expr, new_car);
      set_cdr(expr, new_cdr);
      return expr;
    }
  } else if (::is_void(expr)) {
    return 0;
  } else {
    return expr;
  }
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

size_t interpreter_t::list_length_internal(expr_t* expr) {
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
      throw expr_exception_t("list_length_internal: list is circular", expr);
    }
  }
  return result;
}

expr_t* interpreter_t::list_length(expr_t* expr) {
  return memory.make_integer(list_length_internal(expr));
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

