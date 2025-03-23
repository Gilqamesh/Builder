#include "lexer.h"

struct token_stream_t {
  token_stream_t(const char* source) {
    lexer_t lexer(source);
    token_t token = lexer.eat_token();
    while (token.type != token_type_t::END_OF_FILE) {
      tokens.push(token);
      token = lexer.eat_token();
    }
  }
  queue<token_t> tokens;
  token_t eat() {
    token_t result = tokens.peak();
    tokens.pop();
    return result;
  }
  token_t eat_err(token_type_t token_type) {
    token_t result = tokens.eat();
    if (result.type != token_type) {
      throw runtime_error(string("token_stream_t::eat_err: expect ") + token_type_to_str(token_type));
    }
    return result;
  }
  token_t peak() const {
    if (is_empty()) {
      throw runtime_error("token_stream_t::peak: no more tokens");
    }
    return tokens.front();
  }
  bool is_empty() const {
    return tokens.empty();
  }
};

enum expr_type_t {
  SELF_EVAL,
  LIST,
  DEFINE,
  LAMBDA
};
 
struct expr_t {
  expr_t(expr_type_t type, token_t token): type(type), token(token) {}

  expr_type_t type;
  token_t token;
};

struct expr_self_eval_t {
  expr_self_eval_t(const token_t& token): base(SELF_EVAL, token) {}

  expr_t base;
};

struct expr_list_t {
  expr_list_t(const token_t& token, const vector<expr_t*>& args): base(LIST, token), args(args) {}

  expr_t base;
  vector<expr_t*> args;
};

struct expr_lambda_t {
  expr_lambda_t(const token_t& token, const vector<expr_t*>& params, expr_t* body): base(LAMBDA, token), params(params), body(body) {}

  expr_t base;
  vector<expr_t*> params;
  expr_t* body;
};

struct expr_define_t {
  expr_define_t(const token_t& token, expr_t* var, expr_t* value): base(DEFINE, token), var(var), value(value) {}
  expr_define_t(const token_t& token, expr_t* var, const vector<expr_t*>& params, expr_t* body): base(DEFINE, token), var(var), value((expr_t*) new expr_lambda_t(var->token, params, body)) {}

  expr_t base;
  expr_t* var;
  expr_t* value;
};

struct pattern_t {
  string pattern;
  function<expr_t*(const vector<token_t>&)> handler;
};
vector<pattern_t> patterns;

void install_pattern(const string& pattern, const function<expr_t*(const vector<token_t>& tokens)>& handler) {
  patterns.push_back({ pattern, handler });
}

expr_t* parse_expr(token_stream_t& token_stream) {
  // look in patterns and select first match
  return 0;
}

expr_t* eat_expr() {
}

expr_t* parse_expr_define() {
  // (define var expr)
  eat(token_type_t::LEFT_PAREN);
  token_t define_token = eat(token_type_t::DEFINE);
  token_t var_token = eat(token_type_t::IDENTIFIER);
  expr_t* expr = eat_expr();
  eat(token_type_t::RIGHT_PAREN);
  new expr_define_t(define_token, var_token, expr);
}

int main() {
  string buf;
  {
    string line;
    while (getline(cin, line)) {
      buf += line + '\n';
    }
  }
  token_stream_t token_stream(buf.c_str());
  install_pattern("list ...", [](token_stream_t& token_stream) -> expr_t* {
    token_t list_token = token_stream.eat_err(token_type_t::LIST);
    while (token_stream.) {
    }
    token_stream.peak()
    parse_expr(token_stream);
    return new expr_list_t(token_stream.eat(), parse_expr(token_stream));
  });
  install_pattern("lambda (...) expr", [](const vector<token-t>& tokens) -> expr_t* {
  });
  expr_t* expr = parse_expr(token_stream);

  return 0;
}

