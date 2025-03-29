#include "evaluator.h"

env_t::env_t():
  next(0)
{
}

env_t env_t::extend(const vector<expr_t*>& vars, const vector<expr_t*>& vals) const {
  env_t result;
  if (vars.size() != vals.size()) {
    throw exception();
  }
  for (int i = 0; i < vars.size(); ++i) {
    expr_t* var = vars[i];
    if (var->type != expr_type_t::VARIABLE) {
      throw exception();
    }
    result.bindings[vars[i]->to_string()] = vals[i];
  }
  result.next = this;
  return result;
}

expr_t* env_t::lookup(const string& var) const {
  return lookup_internal(var)->second;
}

expr_t* env_t::set(const string& var, expr_t* expr) {
  auto it = lookup_internal(var);
  it->second = expr;
  return expr;
}

expr_t* env_t::define(const string& var, expr_t* expr) {
  bindings[var] = expr;
  return expr;
}

map<string, expr_t*>::iterator env_t::lookup_internal(const string& var) {
  auto it = bindings.find(var);
  if (it == bindings.end()) {
    if (!next) {
      throw exception();
    }
    return next->lookup(var);
  }
  return it;
}

evaluator_t::evaluator_t() {
  global_env.define("nil", (expr_t*) new expr_nil_t());
  global_env.define("car", (expr_t*) new obj_primitive_proc_t([](const vector<obj_t*>& args) {
    if (args.empty()) {
      throw exception();
    }
    if (is_false(apply_primitive_procedure(global_env.lookup("pair?"), args))) {
      throw exception();
    }
    return ((obj_pair_t*)obj)->first;
  }));
  global_env.define("cdr", (obj_t*) new obj_primitive_proc_t([this](const vector<obj_t*>& args) {
    if (args.empty()) {
      throw exception();
    }
    if (is_false(apply_primitive_procedure(global_env.lookup("pair?"), args))) {
      throw exception();
    }
    return ((obj_pair_t*)args[0])->second;
  }));
  global_env.define("cons", (obj_t*) new obj_primitive_proc_t([](const vector<obj_t*>& args) {
    return (obj_t*) new obj_pair_t(obj1, obj2);
  }));
  global_env.define("list", (obj_t*) new obj_primitive_proc_t([](const vector<obj_t*>& args) {
    if (args.empty()) {
      return (obj_t*) new obj_nil_t();
    }
    obj_t* result = (obj_t*) new obj_nil_t();
    for (int i = args.size() - 1; 0 <= i; --i) {
      result = apply_primitive_procedure(global_env.lookup("cons"), { args[i], result });
    }
    return result;
  }));
  global_env.define("null?", (obj_t*) new obj_primitive_proc_t([this](const vector<obj_t*>& args) {
    return global_env.lookup("nil") == obj;
  }));
  global_env.define("pair?", (obj_t*) new obj_primitive_proc_t([](const vector<obj_t*>& args) {
    if (args.empty()) {
      throw exception();
    }
    return args[0]->type == obj_type_t::PAIR;
  }));
}

expr_t* evaluator_t::eval(expr_t* expr) {
  return eval(expr, global_env);
}

expr_t* evaluator_t::eval(expr_t* expr, env_t env) {
  switch (expr->type) {
  case expr_type_t::NIL:
  case expr_type_t::NUMBER:
  case expr_type_t::STRING: return expr;
  case expr_type_t::SYMBOL: return env.lookup(((expr_symbol_t*)expr)->symbol);
  case expr_type_t::LIST: {
    expr_list_t* expr_list = (expr_list_t*)expr;
    exprs;
  } break ;
  default: assert(0);
  }

  case SELF_EVALUATING: return eval_self_evaluating(expr);
  case VARIABLE: return eval_variable(expr, env);
  case QUOTED: return eval_quoted(expr);
  case ASSIGNMENT: return eval_assignment(expr, env);
  case DEFINITION: return eval_definition(expr, env);
  case IF: return eval_if(expr, env);
  case LAMBDA: return eval_lambda(expr, env);
  case BEGIN: return eval_begin(expr, env);
  case APPLICATION: return eval_application(expr, env);
  default: throw exception();
  }
}

expr_t* evaluator_t::eval_self_evaluating(expr_t* expr) {
  switch (expr->token.type) {
  case token_type_t::NUMBER: return (expr_t*) new obj_number_t(stod(expr->token.to_string()));
  case token_type_t::STRING: return (expr_t*) new obj_string_t(expr->token.to_string()); // maybe without quotes
  default: expr->print(); assert(0 && "unexpected token type for self evaluating expression");
  }
}

obj_t* evaluator_t::eval_variable(expr_t* expr, env_t env) {
  return env.lookup(expr->to_string());
}

obj_t* evaluator_t::eval_quoted(expr_t* expr) {
  return (obj_t*) new obj_expr_t(((expr_quoted_t*)expr)->quoted_expr);
}

obj_t* evaluator_t::eval_assignment(expr_t* expr, env_t env) {
  expr_assignment_t* assignment = (expr_assignment_t*)expr;
  return env.set(assignment->lvalue->to_string(), eval(assignment->new_value, env));
}

obj_t* evaluator_t::eval_definition(expr_t* expr, env_t env) {
  expr_define_t* def = (expr_define_t*) expr;
  return env.define(def->variable->to_string(), eval(def->value, env));
}

obj_t* evaluator_t::eval_if(expr_t* expr, env_t env) {
  expr_if_t* expr_if = (expr_if_t*)expr;
  if (is_true(eval(expr_if->condition, env))) {
    return eval(expr_if->consequence, env);
  } else if (expr_if->alternative) {
    return eval(expr_if->alternative, env);
  } else {
    return (obj_t*) new obj_nil_t();
  }
}

obj_t* evaluator_t::eval_lambda(expr_t* expr, env_t env) {
  expr_lambda_t* expr_lambda = (expr_lambda_t*)expr;
  return (obj_t*) new obj_compound_proc_t(expr_lambda->parameters, expr_lambda->body, env);
}

obj_t* evaluator_t::eval_begin(expr_t* expr, env_t env) {
  expr_begin_t* expr_begin = (expr_begin_t*)expr;
  obj_t* result = 0;
  for (expr_t* expression : expr_begin->expressions) {
    result = eval(expression, env);
  }
  return result;
}

obj_t* evaluator_t::eval_application(expr_t* expr, env_t env) {
  expr_application_t* expr_application = (expr_application_t*)expr;
  vector<obj_t*> args;
  for (expr_t* param : expr_application->operands) {
    args.push_back(eval(param, env));
  }
  return apply(eval(expr_application->fn_to_apply, env), args);
}

obj_t* evaluator_t::apply(obj_t* proc, const vector<obj_t*>& args) {
  switch (proc->type) {
  case PRIMITIVE_PROC: return apply_primitive_proc(proc, args);
  case COMPOUND_PROC: return apply_compound_proc(proc, args);
  default: throw exception();
  }
}

expr_t* evaluator_t::apply_primitive_proc(expr_t* expr, const vector<expr_t*>& args) {
  // lookup proc, execute it with args
}

expr_t* evaluator_t::apply_compound_proc(expr_t* obj, const vector<expr_t*>& args) {
  obj_compound_proc_t* proc = (obj_compound_proc_t*)obj;
  env_t extended_env = proc->env.extend(proc->params, args)
  obj_t* result = 0;
  for (expr_t* expr : proc->body) {
    result = eval(expr, extended_env);
  }
  return result;
}

bool evaluator_t::is_true(obj_t* obj) {
  return obj->type != obj_type_t::NIL;
}

bool evaluator_t::is_false(obj_t* obj) {
  return obj->type == obj_type_t::NIL;
}


