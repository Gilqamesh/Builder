#include "evaluator.h"

const char* obj_type_to_str(obj_type_t obj_type) {
}

obj_t::obj_t(obj_type_t type):
  type(type)
{
}

env_t::env_t():
  next(0)
{
}

env_t env_t::extend(const vector<expr_t*>& vars, const vector<obj_t*>& vals) const {
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

obj_t* env_t::lookup(const string& var) const {
  return lookup_internal(var)->second;
}

void env_t::set(const string& var, obj_t* obj) {
  auto it = lookup_internal(var);
  it->second = obj;
}

void env_t::define(const string& var, obj_t* obj) {
  bindings[var] = obj;
}

map<string, obj_t*>::iterator env_t::lookup_internal(const string& var) {
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
  global_env.define("nil", (obj_t*) new obj_nil_t());
  global_env.define("car", (obj_t*) new obj_primitive_proc_t([](const vector<obj_t*>& args) {
    if (is_false(apply_primitive_procedure(global_env.lookup("pair?"), obj))) {
      throw exception();
    }
    return ((obj_pair_t*)obj)->first;
  }));
  global_env.define("cdr", (obj_t*) new obj_primitive_proc_t([this](const vector<obj_t*>& args) {
    if (is_false(apply_primitive_procedure(global_env.lookup("pair?"), obj))) {
      throw exception();
    }
    return ((obj_pair_t*)obj)->second;
  }));
  global_env.define("cons", (obj_t*) new obj_primitive_proc_t([](const vector<obj_t*>& args) {
    return (obj_t*) new obj_pair_t(obj1, obj2);
  }));
  global_env.define("null?", (obj_t*) new obj_primitive_proc_t([this]() {
    return global_env.lookup("nil") == obj;
  }));
  global_env.define("pair?", (obj_t*) new obj_primitive_proc_t([](obj_t* obj) {
    return obj->type == obj_type_t::PAIR;
  }));
}

obj_t* evaluator_t::eval(expr_t* expr) {
  return eval(expr, global_env);
}

obj_t* evaluator_t::eval(expr_t* expr, env_t env) {
  switch (expr->type) {
  }
}

obj_t* evaluator_t::apply(obj_t* proc, const vector<obj_t*>& args) {
  switch (proc->type) {
  case PRIMITIVE_PROC: return apply_primitive_proc(proc, args);
  case COMPOUND_PROC: return apply_compound_proc(proc, args);
  default: throw exception();
  }
}

bool evaluator_t::is_true(obj_t* obj) {
  return obj->type != obj_type_t::NIL;
}

bool evaluator_t::is_false(obj_t* obj) {
  return obj->type == obj_type_t::NIL;
}

obj_t* evaluator_t::apply_primitive_proc(obj_t* obj, const vector<obj_t*>& args) {
  return ((obj_primitive_proc_t*)obj)->proc(args);
}

obj_t* evaluator_t::apply_compound_proc(obj_t* obj, const vector<obj_t*>& args) {
  obj_compound_proc_t* proc = (obj_compound_proc_t*)obj;
  env_t extended_env = proc->env.extend(proc->params, args)
  obj_t* result = 0;
  for (expr_t* expr : proc->body) {
    result = eval(expr, extended_env);
  }
  return result;
}

bool is_self_evaluating(expr_t* expr) {
  if (is_number(expr)) {
    return true;
  } else if (is_string(expr)) {
    return true;
  }
  return false;
}

bool is_variable(expr_t* expr) {
  if (is_symbol(expr)) {
    return true;
  }
  return false;
}

bool is_assignment(expr_t* expr) {
}

list_of_values(expressions, env_t* env) {
  result;
  for (expr_t* expr : expressions) {
    result.push_back(eval(expr, env));
  }
  return result;
}

eval_if(expr_t* expr, env_t* env) {
  if (is_true(eval(if_predicate(expr), env))) {
    return eval(if_consequent(expr), env);
  } else {
    return eval(if_alternative(expr), env);
  }
}

eval_sequence(expressions, env_t* env) {
  last;
  for (expr_t* expr : expressions) {
    last = eval(expr, env);
  }
  return last;
}

eval_assignment(expr_t* expr, env_t* env) {
  new_value = eval(assignment_value(expr), env);
  env->set(assignment_variable(expr), new_value);
  return new_value;
}

eval_definition(expr_t* expr, env_t* env) {
  defined_value = eval(definition_value(expr), env);
  env->define(definition_variable(expr), defined_value);
  return defined_value;
}

eval(expr_t* expr, env_t* env) {
  switch (expr->type) {
  case SELF_EVALUATING: return expr;
  case VARIABLE: return env->lookup(expr);
  case QUOTE: return text_of_quotation(expr);
  case ASSIGNMENT: return eval_assignment(expr, env);
  case DEFINITION: return eval_definition(expr, env);
  case IF: return eval_if(expr, env);
  case LAMBDA: return make_procedure(lambda_parameters(expr), lambda_body(expr), env);
  case BEGIN: return eval_sequence(begin_actions(expr), env);
  case COND: return eval(cond_to_if(epxr), env);
  case APPLICATION: return apply(eval(expr->get_operator(), env), list_of_values(expr->get_operands(), env));
  default: assert(0);
  }
}

apply(procedure, arguments) {
  if (is_primitive_procedure(procedure)) {
    return apply_primitive_procedure(procedure, arguments);
  } else if (is_compund_procedure(procedure)) {
    return eval_sequence(procedure->body(), extend_environment(procedure->arguments(), arguments, procedure->environment()));
  } else { 
    assert(0);
  }
}

