#ifndef EXPR_H
# define EXPR_H

# include "libc.h"

enum class expr_type_t : int {
  END_OF_FILE,
  NIL,
  VOID,
  BOOLEAN,
  CHAR,
  INTEGER,
  REAL,
  STRING,
  SYMBOL,
  ENV,
  PRIMITIVE,
  MACRO,
  CONS,
  ISTREAM,
  OSTREAM
};

const char* expr_type_to_str(expr_type_t expr_type);

struct expr_t {
  expr_t(expr_type_t type);

  expr_type_t type;
};

struct expr_eof_t {
  expr_eof_t();

  expr_t base;
};

struct expr_nil_t {
  expr_nil_t();

  expr_t base;
};

struct expr_void_t {
  expr_void_t();

  expr_t base;
};

struct expr_boolean_t {
  expr_boolean_t(bool boolean);

  expr_t base;
  bool boolean;
};

struct expr_char_t {
  expr_char_t(char c);

  expr_t base;
  char c;
};

struct expr_integer_t {
  expr_integer_t(int64_t integer);

  expr_t base;
  int64_t integer;
};

struct expr_real_t {
  expr_real_t(double real);

  expr_t base;
  double real;
};

struct expr_string_t {
  expr_string_t(const string& str);

  expr_t base;
  string str;
};

struct expr_symbol_t {
  expr_symbol_t(string symbol);

  expr_t base;
  string symbol;
};

struct expr_env_t {
  expr_env_t();

  expr_t base;
  unordered_map<expr_t*, expr_t*> bindings;
  expr_t* parent;

  expr_t* get(expr_t* symbol);
  expr_t* set(expr_t* symbol, expr_t* expr);
  expr_t* define(expr_t* symbol, expr_t* expr);
};

struct expr_macro_t {
  expr_macro_t(function<expr_t*(expr_t* args, expr_t* call_env)> f);

  expr_t base;
  function<expr_t*(expr_t* args, expr_t* call_env)> f;
};

struct expr_cons_t {
  expr_cons_t(expr_t* first, expr_t* second);

  expr_t base;
  expr_t* first;
  expr_t* second;
};

struct expr_istream_t {
  expr_istream_t(istream& is);
  expr_istream_t(unique_ptr<istream> is);

  expr_t base;
  unique_ptr<istream, function<void(istream*)>> is;
};

struct expr_ostream_t {
  expr_ostream_t(ostream& os);
  expr_ostream_t(unique_ptr<ostream> os);

  expr_t base;
  unique_ptr<ostream, function<void(ostream*)>> os;
};

struct expr_exception_t : public exception {
  expr_exception_t(const string& message, expr_t* expr);

  expr_t* expr;
  string message;

  const char* what() const noexcept override;
};

bool is_eof(expr_t* expr);

bool is_nil(expr_t* expr);

bool is_void(expr_t* expr);

bool is_boolean(expr_t* expr);
bool get_boolean(expr_t* expr);

bool is_char(expr_t* expr);
char get_char(expr_t* expr);

bool is_cons(expr_t* expr);
expr_t* car(expr_t* expr);
expr_t* cdr(expr_t* expr);
void set_car(expr_t* expr, expr_t* val);
void set_cdr(expr_t* expr, expr_t* val);

bool is_integer(expr_t* expr);
int64_t get_integer(expr_t* expr);

bool is_real(expr_t* expr);
double get_real(expr_t* expr);

bool is_string(expr_t* expr);
string get_string(expr_t* expr);

bool is_symbol(expr_t* expr);
string get_symbol(expr_t* expr);

bool is_env(expr_t* expr);
expr_t* env_get_parent(expr_t* env);
void env_set_parent(expr_t* env, expr_t* new_parent);
expr_t* env_define(expr_t* env, expr_t* symbol, expr_t* expr);
expr_t* env_get(expr_t* env, expr_t* symbol);
expr_t* env_set(expr_t* env, expr_t* symbol, expr_t* expr);
unordered_map<expr_t*, expr_t*>& get_env_bindings(expr_t* env);

bool is_macro(expr_t* expr);
expr_t* macro_def_env(expr_t* expr);
const function<expr_t*(expr_t* args, expr_t* call_env)>& macro_f(expr_t* expr);

bool is_istream(expr_t* expr);
istream& get_istream(expr_t* expr);

bool is_ostream(expr_t* expr);
ostream& get_ostream(expr_t* expr);

// ---

// todo: move to interprer
string to_string(expr_t* expr);
void print(expr_t* expr, ostream& os = cout);

bool is_list(expr_t* expr);

#endif // EXPR_H

