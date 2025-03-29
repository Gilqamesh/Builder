#ifndef EVALUATOR_H
# define EVALUTOR_H

# include "parser.h"

struct env_t {
  env_t();

  map<string, expr_t*> bindings;
  env_t* next;

  env_t extend(const vector<expr_t*>& vars, const vector<expr_t*>& vals) const;
  expr_t* lookup(const string& var) const;
  expr_t* set(const string& var, expr_t* expr);
  expr_t* define(const string& var, expr_t* expr);

private:
  map<string, expr_t*>::iterator lookup_internal(const string& var);
};

struct evaluator_t {
  evaluator_t();

  expr_t* eval(expr_t* expr);

private:
  env_t global_env;

  expr_t* eval(expr_t* expr, env_t env);
  expr_t* eval_self_evaluating(expr_t* expr);
  expr_t* eval_cons(expr_t* expr, env);
  expr_t* eval_car(expr_t* expr, env);
  expr_t* eval_cdr(expr_t* expr, env);
  expr_t* eval_list(expr_t* expr, env);
  expr_t* eval_variable(expr_t* expr, env_t env);
  expr_t* eval_quoted(expr_t* expr);
  expr_t* eval_assignment(expr_t* expr, env_t env);
  expr_t* eval_definition(expr_t* expr, env_t env);
  expr_t* eval_if(expr_t* expr, env_t env);
  expr_t* eval_lambda(expr_t* expr, env_t env);
  expr_t* eval_begin(expr_t* expr, env_t env);
  expr_t* eval_application(expr_t* expr, env_t env);

  expr_t* apply(expr_t* proc, const vector<expr_t*>& args);
  expr_t* apply_primitive_proc(expr_t* expr, const vector<expr_t*>& args);
  expr_t* apply_compound_proc(expr_t* expr, const vector<expr_t*>& args);

  bool is_true(expr_t* expr);
  bool is_false(expr_t* expr);
};

#endif // EVALUATOR_H

