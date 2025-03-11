#include "evaluator.h"

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

