#ifndef EVALUATOR_H
# define EVALUTOR_H

# include "parser.h"

enum class obj_type_t : int {
  NIL,
  PAIR,
  PRIMITIVE_PROC,
  COMPOUND_PROC
};

const char* obj_type_to_str(obj_type_t obj_type);

struct obj_t {
  obj_t(obj_type_t type);

  obj_type_t type;
};

struct env_t {
  env_t();

  map<string, obj_t*> bindings;
  env_t* next;

  env_t extend(const vector<expr_t*>& vars, const vector<obj_t*>& vals) const;
  obj_t* lookup(const string& var) const;
  void set(const string& var, obj_t* obj);
  void define(const string& var, obj_t* obj);

private:
  map<string, obj_t*>::iterator lookup_internal(const string& var);
};

struct obj_nil_t {
  obj_nil_t();

  obj_t base;
};

struct obj_pair_t {
  obj_pair_t(obj_t* first, obj_t* second);

  obj_t base;

  obj_t* first;
  obj_t* second;
};

struct obj_primitive_proc_t {
  obj_primitive_proc_t(const function<obj_t*(const vector<obj_t*>&)>&);

  obj_t base;

  function<obj_t*(const vector<obj_t*>&)> proc;
};

struct obj_compound_proc_t {
  obj_compound_proc_t(const vector<expr_t*>& params, const vector<expr_t*>& body, env_t env);

  obj_t base;

  vector<expr_t*> params;
  vector<expr_t*> body;
  env_t env;
};

struct evaluator_t {
  evaluator_t();

  obj_t* eval(expr_t* expr);

private:
  env_t global_env;

  obj_t* eval(expr_t* expr, env_t env);
  obj_t* apply(obj_t* proc, const vector<obj_t*>& args);

  bool is_true(obj_t* obj);
  bool is_false(obj_t* obj);
  obj_t* apply_primitive_proc(obj_t* obj, const vector<obj_t*>& args);
  obj_t* apply_compound_proc(obj_t* obj, const vector<obj_t*>& args);
};

#endif // EVALUATOR_H

