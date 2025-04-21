#include "interpreter.h"

interpreter_t::interpreter_t():
  reader([this](const string& lexeme) {
    return make_symbol(lexeme);
  })
{
  reader.set_skippable_predicate([](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
  });
  nil = (expr_t*) new expr_nil_t();
  t = make_symbol("#t");
  void_expr = (expr_t*) new expr_void_t();
  for (int i = 0; i < sizeof(chars) / sizeof(chars[0]); ++i) {
    chars[i] = (expr_t*) new expr_char_t((char)i);
  }
  char_map = {
    { "space", chars[' '] },
    { "backspace", chars[8] },
    { "newline", chars['\n'] },
    { "tab", chars['\t'] }
  };
  
  global_env.define(t, t);

  reader.add("(", [this](reader_t& reader, const string& lexeme, istream& is) {
    expr_t* first = 0;
    expr_t* prev = 0;
    while (expr_t* expr = reader.read(/* reader env */ is, [](reader_t& reader, istream& is) {  })) {
      assert(expr);
      expr_t* cur = make_cons(expr, make_nil());
      if (!first) {
        first = cur;
      } else {
        set_cdr(prev, cur);
      }
      prev = cur;
    }
    if (reader.eat(is) != ')') {
      throw runtime_error("expect ')'");
    }
    if (first) {
      return first;
    }
    return make_nil();
  });

  reader.add("#|", [this](reader_t& reader, const string& lexeme, istream& is) {
    while (!reader.is_at_end(is)) {
      if (reader.eat(is) == '|') {
        if (reader.peek(is) == '#') {
          reader.eat(is);
          return make_void();
        }
      }
    }
    throw runtime_error("expect |#");
  });

  reader.add("#\\", [this](reader_t& reader, const string& lexeme, istream& is) {
    while (!reader.is_at_end(is) && ) {
      if () {
      }
    }
    expr_t* expr = reader.read(is);
    if (!is_symbol(expr)) {
      throw expr_exception_t("#\\: expect symbol", expr);
    }
    string symbol = get_symbol(expr);
    if (symbol.size() == 1) {
      return chars[symbol[0]];
    } else {
      auto it = char_map.find(symbol);
      if (it != char_map.end()) {
        return it->second;
      }
    }
    throw expr_exception_t("#\\: unknown character name", expr);
  });

  reader.add("\"", [this](reader_t& reader, const string& lexeme, istream& is) {
    while (!reader.is_at_end(is)) {
      if (reader.eat(is) == '"') {
        return make_string("\"" + reader.lexeme());
      }
    }
    throw runtime_error("\": expect ending \"");
  });
  function<expr_t*(reader_t&, const string&, istream& is)> number_reader = [](reader_t& reader, const string& lexeme, istream& is) {
    
  };

 // case token_type_t::CHAR: return read_char(reader_env, token);
 // case token_type_t::NUMBER: return read_number(reader_env, token);
 // case token_type_t::STRING: return read_string(reader_env, token);
 // case token_type_t::NIL: return read_nil(reader_env);
 // case token_type_t::ERROR: throw token_exception_t("error", token);
 // case token_type_t::COMMENT: return read(reader_env, lexer, eat_token(lexer));
 //
  reader.add("'", [this](reader_t& reader, const string& symbol, istream& is) {
    return make_list({ make_symbol("quote"), reader.read(is) });
  });
  reader.add("`", [this](reader_t& reader, const string& symbol, istream& is) {
    return make_list({ make_symbol("quasiquote"), reader.read(is) });
  });
  reader.add(",", [this](reader_t& reader, const string& symbol, istream& is) {
    return make_list({ make_symbol("unquote"), reader.read(is) });
  });
  reader.add(",@", [this](reader_t& reader, const string& symbol, istream& is) {
    return make_list({ make_symbol("unquote-splicing"), reader.read(is) });
  });

  global_env.define(make_symbol("cin"), make_istream(cin));
  global_env.define(make_symbol("cout"), make_ostream(cout));
  global_env.define(make_symbol("car"), make_primitive_proc([this](expr_t* expr) { return car(car(expr)); }, 1, false));
  global_env.define(make_symbol("cdr"), make_primitive_proc([this](expr_t* expr) { return cdr(car(expr)); }, 1, false));
  global_env.define(make_symbol("cons"), make_primitive_proc([this](expr_t* expr) { return make_cons(car(expr), car(cdr(expr))); }, 2, false));
  global_env.define(make_symbol("list"), make_primitive_proc([this](expr_t* expr) { return expr; }, 0, true));

  global_env.define(make_symbol("eq?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_eq(car(expr), car(cdr(expr)))); }, 2, false));
  global_env.define(make_symbol("symbol?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_symbol(car(expr))); }, 1, false));
  global_env.define(make_symbol("string?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_string(car(expr))); }, 1, false));
  global_env.define(make_symbol("list?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_list(car(expr))); }, 1, false));
  global_env.define(make_symbol("null?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_nil(car(expr))); }, 1, false));
  global_env.define(make_symbol("integer?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_integer(car(expr))); }, 1, false));
  global_env.define(make_symbol("pair?"), make_primitive_proc([this](expr_t* expr) { return make_boolean(is_cons(car(expr))); }, 1, false));

  global_env.define(make_symbol("display"), make_primitive_proc([this](expr_t* expr) {
    expr = car(expr);
    if (is_string(expr)) {
      string s = expr->to_string();
      cout << s.substr(1, s.size() - 2);
    } else {
      cout << expr->to_string();
    }
    return make_void();
  }, 1, false));

  global_env.define(make_symbol("+"), make_primitive_proc([this](expr_t* expr) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_add(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("-"), make_primitive_proc([this](expr_t* expr) {
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_sub(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("*"), make_primitive_proc([this](expr_t* expr) {
    expr_t* result = make_real(1.0);
    while (!is_nil(expr)) {
      result = number_mul(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("/"), make_primitive_proc([this](expr_t* expr) {
    if (list_length_internal(expr) == 1) {
      return number_div(make_real(1.0), car(expr));
    }
    expr_t* result = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      result = number_div(result, car(expr));
      expr = cdr(expr);
    }
    return result;
  }, 1, true));
  global_env.define(make_symbol("="), make_primitive_proc([this](expr_t* expr) {
    expr_t* first = car(expr);
    expr = cdr(expr);
    while (!is_nil(expr)) {
      if (is_false(number_eq(first, car(expr)))) {
        return make_boolean(false);
      }
      expr = cdr(expr);
    }
    return make_boolean(true);
  }, 1, true));
  global_env.define(make_symbol("<"), make_primitive_proc([this](expr_t* expr) {
    return make_boolean(get_real(list_ref(expr, 0)) < get_real(list_ref(expr, 1)));
  }, 2, false));

  global_env.define(make_symbol("read"), make_primitive_proc([this](expr_t* expr) { return read(list_ref(expr, 0)); }, 1, false));
  global_env.define(make_symbol("eval"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    int len = list_length_internal(expr);
    if (len == -1) {
      throw expr_exception_t("eval: list has cycle", expr);
    }
    if (len == 1) {
      return eval(eval(car(expr), env), env);
    } else if (len == 2) {
      return eval(eval(car(expr), env), env);
    } else {
      throw expr_exception_t("eval: wrong arity for special form", expr);
    }
  }));
  global_env.define(make_symbol("apply"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    // (apply fn args)
    int len = list_length_internal(expr);
    if (len == -1) {
      throw expr_exception_t("apply: list has cycle", expr);
    }
    if (len != 2) {
      throw expr_exception_t("apply: wrong arity for special form", expr);
    }
    return apply(eval(list_ref(expr, 0), env), eval(list_ref(expr, 1), env), env);
  }));

  global_env.define(make_symbol("begin"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    expr_t* result = make_nil();
    while (!is_nil(expr)) {
      result = eval(car(expr), env);
      expr = cdr(expr);
    }
    return result;
  }));
  global_env.define(make_symbol("quote"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    // 'expr
    return car(expr);
  }));
  // global_env.define(make_symbol("unquote"), make_special_form([this](expr_t* expr, expr_env_t* env) {
  //   // ,expr
  //   return car(expr);
  // }));

  //global_env.define(make_symbol("quasiquote"), make_special_form([this](expr_t* expr, expr_env_t* env) {
  //  return quasiquote(car(expr), env);
  //}));

  // global_env.define(make_symbol("unquote-splicing"), make_special_form([this](expr_t* expr, expr_env_t* env) {
  //   // ,@expr
  //   return car(expr);
  // }));

  global_env.define(make_symbol("if"), make_special_form([this](expr_t* expr, expr_env_t* env) {
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
      return make_nil();
    }
  }));
  global_env.define(make_symbol("defreader"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    expr_t* symbol = list_ref(expr, 0);
    expr_t* body = list_ref(expr, 1);
    reader.add(get_symbol(symbol), [this, body](reader_t& reader, const string&, istream& is) {
      return eval(body);
    });
    return body;
  }));
  global_env.define(make_symbol("defmacro"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    // (defmacro symbol (params) expand-to)
    return env->define(list_ref(expr, 0), make_macro([this, env, expr](expr_t* args) {
      expr_env_t extended_env = extend_env(env, list_ref(expr, 1), args);
      return eval(list_ref(expr, 2), &extended_env);
    }));
  }));
  global_env.define(make_symbol("cond"), make_macro([this](expr_t* expr) {
    // (cond <(condition consequence)>+ [consequence_last]) -> (if condition (if condition) [consequence_last])
    expr_t* result = make_nil();
    while (!is_nil(expr)) {
      expr_t* cur = car(expr);
      expr = cdr(expr);
    }
    return result;
  }));
  global_env.define(make_symbol("macroexpand"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    return macroexpand(eval(car(expr)), env);
  }));
  global_env.define(make_symbol("macroexpand-all"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    return macroexpand_all(eval(car(expr)), env);
  }));

  global_env.define(make_symbol("define"), make_special_form([this](expr_t* expr, expr_env_t* env) {
    int len = list_length_internal(expr);
    if (len == -1) {
      throw expr_exception_t("define: cycle in list", expr);
    }
    expr_t* first = list_ref(expr, 0);
    expr_t* rest = cdr(expr);
    if (is_list(first)) {
      // (define (fn args ...) body)
      return env->define(car(first), make_compound_proc(cdr(first), rest));
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

  global_env.define(make_symbol("lambda"), make_special_form([this](expr_t* expr, expr_env_t* env) {
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
    return make_compound_proc(params, body);
  }));
}

static token_t eat_token(lexer_t& lexer) {
  token_t token = lexer.eat_token();
  while (token.type == token_type_t::COMMENT) {
    token = lexer.eat_token();
  }
  if (token.type == token_type_t::ERROR) {
    throw token_exception_t("unexpected error token", token);
  }
  return token;
}

expr_t* interpreter_t::read(istream& is) {
 //lexer_t lexer(is);
 //return read(&global_reader_env, lexer, eat_token(lexer));
 return reader.read(is);
}

void interpreter_t::print(ostream& os, expr_t* expr) {
  os << expr->to_string() << endl;
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
      if (expr_t* expr = read(is)) {
        expr->print(os);
        print(os, eval(expr));
      } else {
        break ;
      }
    } catch (token_exception_t& e) {
      cerr << "exception: " << e.what() << " " << e.token << endl;
    } catch (expr_exception_t& e) {
      cerr << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " <<  e.expr->to_string() << endl;
    }
  }
}

void interpreter_t::source(istream& is) {
  while (is) {
    try {
      if (expr_t* expr = read(is)) {
        eval(expr);
      } else {
        break ;
      }
    } catch (token_exception_t& e) {
      cerr << "exception: " << e.what() << " " << e.token << endl;
    } catch (expr_exception_t& e) {
      cerr << "exception: " << e.what() << " " << expr_type_to_str(e.expr->type) << ": " <<  e.expr->to_string() << endl;
    }
  }
}

expr_t* interpreter_t::read(expr_t* istream) {
  return read(get_istream(istream));
}

expr_t* interpreter_t::read(expr_env_t* reader_env, lexer_t& lexer, token_t token) {
  switch (token.type) {
  case token_type_t::LEFT_PAREN: return read_list(reader_env, lexer);
  case token_type_t::RIGHT_PAREN: throw token_exception_t("unmatched right paren", token);
  case token_type_t::ELLIPSIS: assert(0 && "todo: implement");
  // todo: add set-dispatch-macro-character and set-macro-character
  case token_type_t::APOSTROPHE: return make_cons(make_symbol("quote"), read(reader_env, lexer, eat_token(lexer)));
  case token_type_t::BACKQUOTE: return make_cons(make_symbol("quasiquote"), read(reader_env, lexer, eat_token(lexer)));
  case token_type_t::COMMA: return make_cons(make_symbol("unquote"), read(reader_env, lexer, eat_token(lexer)));
  case token_type_t::COMMA_SPLICE: return make_cons(make_symbol("unquote-splicing"), read(reader_env, lexer, eat_token(lexer)));

  // todo: remove all of these and define them as special forms
  case token_type_t::DEFINE: 
  case token_type_t::LAMBDA:
  case token_type_t::IF:
  case token_type_t::WHEN:
  case token_type_t::COND:
  case token_type_t::LET:
  case token_type_t::BEGIN:
  case token_type_t::SET: return read_symbol(reader_env, lexer, token);

  case token_type_t::IDENTIFIER: return read_symbol(reader_env, lexer, token);
  case token_type_t::CHAR: return read_char(reader_env, token);
  case token_type_t::NUMBER: return read_number(reader_env, token);
  case token_type_t::STRING: return read_string(reader_env, token);
  case token_type_t::NIL: return read_nil(reader_env);
  case token_type_t::ERROR: throw token_exception_t("error", token);
  case token_type_t::COMMENT: return read(reader_env, lexer, eat_token(lexer));
  case token_type_t::END_OF_FILE: return 0;
  default: assert(0);
  }
  return 0;
}

expr_t* interpreter_t::read_nil(expr_env_t* reader_env) {
  return make_nil();
}

expr_t* interpreter_t::read_char(expr_env_t* reader_env, token_t token) {
  expr_t* result = 0;
  if (token.lexeme.size() == 1) {
    result = make_char(token.lexeme[0]);
  } else {
    auto it = char_map.find(token.lexeme);
    if (it != char_map.end()) {
      result = it->second;
    } else {
      throw token_exception_t("unknown character name", token);
    }
  }
  return result;
}

expr_t* interpreter_t::read_number(expr_env_t* reader_env, token_t token) {
  double real = stod(token.lexeme);
  if ((int64_t) real == real) {
    return make_integer((int64_t) real);
  } else {
    return make_real(real);
  }
}

expr_t* interpreter_t::read_string(expr_env_t* reader_env, token_t token) {
  return make_string(token.lexeme);
}

expr_t* interpreter_t::read_symbol(expr_env_t* reader_env, lexer_t& lexer, token_t token) {
  expr_t* symbol = make_symbol(token.lexeme);
  if (expr_t* reader = reader_env->get(symbol)) {
    // return eval(reader);
  }
  return symbol;
}

expr_t* interpreter_t::read_list(expr_env_t* reader_env, lexer_t& lexer) {
  expr_t* first = 0;
  expr_t* prev = 0;
  token_t token = eat_token(lexer);
  while (token.type != token_type_t::END_OF_FILE && token.type != token_type_t::RIGHT_PAREN) {
    expr_t* expr = read(reader_env, lexer, token);
    if (!expr) {
      throw token_exception_t("expect expression", token);
    }
    expr_t* cur = make_cons(expr, make_nil());
    if (!first) {
      first = cur;
    } else {
      set_cdr(prev, cur);
    }
    prev = cur;
    token = eat_token(lexer);
  }
  if (token.type != token_type_t::RIGHT_PAREN) {
    throw token_exception_t("expect right paren", token);
  }
  if (first) {
    return first;
  }
  return make_nil();
}

expr_t* interpreter_t::read_backquote(expr_env_t* reader_env, lexer_t& lexer, token_t token) {
  return 0;
}

expr_t* interpreter_t::eval(expr_t* expr) {
  return eval(expr, &global_env);
}

expr_t* interpreter_t::eval(expr_t* expr, expr_env_t* env) {
  switch (expr->type) {
  case expr_type_t::VOID:
  case expr_type_t::NIL:
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
  case expr_type_t::CONS: return apply(eval(car(expr), env), cdr(expr), env);
  case expr_type_t::ENV:
  case expr_type_t::PRIMITIVE_PROC: 
  case expr_type_t::SPECIAL_FORM:
  case expr_type_t::MACRO:
  case expr_type_t::COMPOUND_PROC: throw expr_exception_t("unexpected type in eval", expr);
  default: assert(0);
  }
}

expr_t* interpreter_t::apply(expr_t* expr, expr_t* args, expr_env_t* env) {
  if (is_special_form(expr)) {
    return apply_special_form(expr, args, env);
  } else if (is_primitive_proc(expr)) {
    return apply_primitive_proc(expr, args, env);
  } else if (is_compound_proc(expr)) {
    return apply_compound_proc(expr, args, env);
  } else if (is_macro(expr)) {
    return apply_macro(expr, args, env);
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
  return proc->f(list_map(args, [this, env](expr_t* expr) {
    return eval(expr, env);
  }));
}

expr_t* interpreter_t::apply_compound_proc(expr_t* expr, expr_t* args, expr_env_t* env) {
  expr_t* body = get_compound_proc_body(expr);
  expr_t* params = get_compound_proc_params(expr);
  expr_env_t extended_env = extend_env(env, params, list_map(args, [this, env](expr_t* expr) {
    return eval(expr, env);
  }));
  if (is_list(body)) {
    expr_t* last = make_nil();
    while (!is_nil(body)) {
      last = eval(car(body), &extended_env);
      body = cdr(body);
    }
    return last;
  } else {
    return eval(body);
  }
}

expr_t* interpreter_t::apply_macro(expr_t* expr, expr_t* args, expr_env_t* env) {
  expr_macro_t* macro = (expr_macro_t*)expr;
  return eval(macro->f(args), env);
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
      return macro->f(cdr(expr));
    }
  }
  return expr;
}

expr_t* interpreter_t::quasiquote(expr_t* expr, expr_env_t* env) {
  expr_t* cur = expr;
  if (is_cons(cur)) {
    expr_t* head = car(cur);
    if (is_symbol(head)) {
      string symbol = get_symbol(head);
      if (symbol == "unquote") {
        set_car(cur, eval(head, env));
      } else if (symbol == "unquote-splicing") {
        // `(1 2 ,@(3 4)) -> (1 2 3 4)
        //set_car();
      } else {
        return expr;
      }
    } else {
      set_car(expr, quasiquote(expr, env));
    }
    if (!is_nil(cdr(expr))) {
      set_cdr(expr, quasiquote(expr, env));
    }
  }
  return expr;
}


expr_t* interpreter_t::make_void() {
  return void_expr;
}

bool interpreter_t::is_void(expr_t* expr) {
  return expr->type == expr_type_t::VOID;
}

expr_t* interpreter_t::make_nil() {
  return nil;
}

bool interpreter_t::is_nil(expr_t* expr) {
  return expr->type == expr_type_t::NIL;
}

expr_t* interpreter_t::make_char(char c) {
  return chars[c];
}

bool interpreter_t::is_char(expr_t* expr) {
  return expr->type == expr_type_t::CHAR;
}

char interpreter_t::get_char(expr_t* expr) {
  if (!is_char(expr)) {
    throw expr_exception_t("expect char", expr);
  }
  return ((expr_char_t*)expr)->c;
}

expr_t* interpreter_t::make_boolean(bool boolean) {
  return boolean ? t : make_nil();
}

bool interpreter_t::is_true(expr_t* expr) {
  return !is_false(expr);
}

bool interpreter_t::is_false(expr_t* expr) {
  return is_nil(expr);
}

expr_t* interpreter_t::make_integer(int64_t integer) {
  return (expr_t*) new expr_integer_t(integer);
}

bool interpreter_t::is_integer(expr_t* expr) {
  return expr->type == expr_type_t::INTEGER;
}

int64_t interpreter_t::get_integer(expr_t* expr) {
  if (!is_integer(expr)) {
    throw expr_exception_t("get_integer: expect integer", expr);
  }
  return ((expr_integer_t*)expr)->integer;
}

expr_t* interpreter_t::make_real(double real) {
  return (expr_t*) new expr_real_t(real);
}

bool interpreter_t::is_real(expr_t* expr) {
  return expr->type == expr_type_t::REAL;
}

double interpreter_t::get_real(expr_t* expr) {
  if (is_integer(expr)) {
    return (double) get_integer(expr);
  } else if (is_real(expr)) {
    return ((expr_real_t*)expr)->real;
  } else {
    throw expr_exception_t("unexpected type in get real", expr);
  }
}

expr_t* interpreter_t::number_add(expr_t* a, expr_t* b) {
  double result = get_real(a) + get_real(b);
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_sub(expr_t* a, expr_t* b) {
  double result = get_real(a) - get_real(b);
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_mul(expr_t* a, expr_t* b) {
  double result = get_real(a) * get_real(b);
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_div(expr_t* a, expr_t* b) {
  double denom = get_real(b);
  if (denom == 0.0) {
    throw expr_exception_t("cannot divide by 0", b);
  }
  double result = get_real(a) / denom;
  if ((int64_t) result == result) {
    return make_integer((int64_t) result);
  } else {
    return make_real(result);
  }
}

expr_t* interpreter_t::number_eq(expr_t* a, expr_t* b) {
  return make_boolean(get_real(a) == get_real(b));
}

expr_t* interpreter_t::make_string(const string& str) {
  auto it = interned_strings.find(str);
  if (it != interned_strings.end()) {
    return it->second;
  }
  expr_t* result = (expr_t*) new expr_string_t(str);
  interned_strings[str] = result;
  return result;
}

bool interpreter_t::is_string(expr_t* expr) {
  return expr->type == expr_type_t::STRING;
}

string interpreter_t::get_string(expr_t* expr) {
  if (!is_string(expr)) {
    throw expr_exception_t("unexpected type in get string", expr);
  }
  return ((expr_string_t*)expr)->str;
}

expr_t* interpreter_t::make_symbol(const string& symbol) {
  auto it = interned_strings.find(symbol);
  if (it != interned_strings.end()) {
    return it->second;
  }
  expr_t* result = (expr_t*) new expr_symbol_t(symbol);
  interned_strings[symbol] = result;
  return result;
}

bool interpreter_t::is_symbol(expr_t* expr) {
  return expr->type == expr_type_t::SYMBOL;
}

string interpreter_t::get_symbol(expr_t* expr) {
  if (!is_symbol(expr)) {
    throw expr_exception_t("unexpected type in get symbol", expr);
  }
  return ((expr_symbol_t*)expr)->symbol;
}

expr_t* interpreter_t::make_env() {
  return (expr_t*) new expr_env_t();
}

expr_env_t interpreter_t::extend_env(expr_env_t* env, expr_t* symbols, expr_t* exprs) {
  expr_env_t result;
  result.parent = env;
  int len_symbols = list_length_internal(symbols);
  int len_exprs = list_length_internal(exprs);
  if (len_symbols == -1) {
    throw expr_exception_t("symbols list has cycle", symbols);
  } else if (len_exprs == -1) {
    throw expr_exception_t("exprs list has cycle", exprs);
  } else if (len_symbols < len_exprs) {
    throw expr_exception_t("symbols list is smaller than exprs list", symbols);
  } else if (len_exprs < len_symbols) {
    throw expr_exception_t("exprs list is smaller than symbols list", exprs);
  }

  for (int i = 0; i < len_symbols; ++i) {
    result.define(car(symbols), car(exprs));
    symbols = cdr(symbols);
    exprs = cdr(exprs);
  }
  return result;
}

expr_t* interpreter_t::make_primitive_proc(const function<expr_t*(expr_t*)>& f, int arity, bool is_variadic) {
  return (expr_t*) new expr_primitive_proc_t(f, arity, is_variadic);
}

bool interpreter_t::is_primitive_proc(expr_t* expr) {
  return expr->type == expr_type_t::PRIMITIVE_PROC;
}

expr_t* interpreter_t::make_special_form(const function<expr_t*(expr_t*, expr_env_t*)>& f) {
  return (expr_t*) new expr_special_form_t(f);
}

expr_t* interpreter_t::make_macro(const function<expr_t*(expr_t*)>& f) {
  return (expr_t*) new expr_macro_t(f);
}

bool interpreter_t::is_macro(expr_t* expr) {
  return expr->type == expr_type_t::MACRO;
}

bool interpreter_t::is_special_form(expr_t* expr) {
  return expr->type == expr_type_t::SPECIAL_FORM;
}

expr_t* interpreter_t::make_compound_proc(expr_t* params, expr_t* body) {
  return (expr_t*) new expr_compound_proc_t(params, body);
}

bool interpreter_t::is_compound_proc(expr_t* expr) {
  return expr->type == expr_type_t::COMPOUND_PROC;
}

expr_t* interpreter_t::get_compound_proc_params(expr_t* expr) {
  if (!is_compound_proc(expr)) {
    throw expr_exception_t("get_compound_proc_params: compound proc expected", expr);
  }
  return ((expr_compound_proc_t*)expr)->params;
}

expr_t* interpreter_t::get_compound_proc_body(expr_t* expr) {
  if (!is_compound_proc(expr)) {
    throw expr_exception_t("get_compound_proc_body: compound proc expected", expr);
  }
  return ((expr_compound_proc_t*)expr)->body;
}

expr_t* interpreter_t::make_cons(expr_t* expr1, expr_t* expr2) {
  return (expr_t*) new expr_cons_t(expr1, expr2);
}

expr_t* interpreter_t::make_list(const initializer_list<expr_t*>& exprs) {
  expr_t* result = make_nil();
  for (auto cur = rbegin(exprs); cur != rend(exprs); ++cur) {
    result = make_cons(*cur, result);
  }
  return result;
}

bool interpreter_t::is_cons(expr_t* expr) {
  return expr->type == expr_type_t::CONS;
}

expr_t* interpreter_t::car(expr_t* expr) {
  if (!is_cons(expr)) {
    throw expr_exception_t("car: cons expected", expr);
  }
  return ((expr_cons_t*)expr)->first;
}

expr_t* interpreter_t::cdr(expr_t* expr) {
  if (!is_cons(expr)) {
    throw expr_exception_t("cdr: cons expected", expr);
  }
  return ((expr_cons_t*)expr)->second;
}

void interpreter_t::set_car(expr_t* expr, expr_t* val) {
  if (!is_cons(expr)) {
    throw expr_exception_t("set_car: cons expected", expr);
  }
  ((expr_cons_t*)expr)->first = val;
}

void interpreter_t::set_cdr(expr_t* expr, expr_t* val) {
  if (!is_cons(expr)) {
    throw expr_exception_t("set_cdr: cons expected", expr);
  }
  ((expr_cons_t*)expr)->second = val;
}

expr_t* interpreter_t::make_istream(istream& is) {
  return (expr_t*) new expr_istream_t(is);
}

bool interpreter_t::is_istream(expr_t* expr) {
  return expr->type == expr_type_t::ISTREAM;
}

istream& interpreter_t::get_istream(expr_t* expr) {
  if (!is_istream(expr)) {
    throw expr_exception_t("get_istream: expect istream", expr);
  }
  return ((expr_istream_t*)expr)->is;
}

expr_t* interpreter_t::make_ostream(ostream& os) {
  return (expr_t*) new expr_ostream_t(os);
}

bool interpreter_t::is_ostream(expr_t* expr) {
  return expr->type == expr_type_t::OSTREAM;
}

ostream& interpreter_t::get_ostream(expr_t* expr) {
  if (!is_ostream(expr)) {
    throw expr_exception_t("get_ostream: expect ostream", expr);
  }
  return ((expr_ostream_t*)expr)->os;
}

bool interpreter_t::is_eq(expr_t* expr1, expr_t* expr2) {
  return expr1 == expr2;
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
    return make_nil();
  }
  return make_integer(result);
}

expr_t* interpreter_t::list_map(expr_t* expr, const function<expr_t*(expr_t*)>& f) {
  if (is_nil(expr)) {
    return make_nil();
  }
  return make_cons(f(car(expr)), list_map(cdr(expr), f));
}

expr_t* interpreter_t::list_reverse(expr_t* expr) {
  expr_t* result = make_nil();
  while (!is_nil(expr)) {
    result = make_cons(car(expr), result);
    expr = cdr(expr);
  }
  return result;
}

bool interpreter_t::is_tagged(expr_t* expr, expr_t* tag) {
  return is_cons(expr) && is_eq(car(expr), tag);
}

bool interpreter_t::is_tagged(expr_t* expr, const string& symbol) {
  return is_cons(expr) && is_symbol(car(expr)) && get_symbol(car(expr)) == symbol;
}


