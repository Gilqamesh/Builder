#ifndef EXPR_H
# define EXPR_H

# include "libc.h"

enum class expr_type_t : int {
  END_OF_FILE,
  VOID,
  NIL,
  BOOLEAN,
  CHAR,
  INTEGER,
  REAL,
  STRING,
  SYMBOL,
  ENV,
  PRIMITIVE_PROC,
  SPECIAL_FORM,
  MACRO,
  COMPOUND_PROC,
  CONS,
  ISTREAM,
  OSTREAM
};

const char* expr_type_to_str(expr_type_t expr_type);

struct expr_t {
  expr_t(expr_type_t type);

  expr_type_t type;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_eof_t {
  expr_eof_t();

  expr_t base;
 
  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_void_t {
  expr_void_t();

  expr_t base;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_nil_t {
  expr_nil_t();

  expr_t base;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_boolean_t {
  expr_boolean_t(bool boolean);

  expr_t base;
  bool boolean;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_char_t {
  expr_char_t(char c);

  expr_t base;
  char c;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_integer_t {
  expr_integer_t(int64_t integer);

  expr_t base;
  int64_t integer;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_real_t {
  expr_real_t(double real);

  expr_t base;
  double real;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_string_t {
  expr_string_t(const string& str);

  expr_t base;
  string str;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_symbol_t {
  expr_symbol_t(string symbol);

  expr_t base;
  string symbol;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_env_t {
  expr_env_t();

  expr_t base;
  unordered_map<expr_t*, expr_t*> bindings;
  expr_env_t* parent;

  expr_t* set(expr_t* symbol, expr_t* expr);
  expr_t* get(expr_t* symbol);
  expr_t* define(expr_t* symbol, expr_t* expr);

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_primitive_proc_t {
  expr_primitive_proc_t(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f);

  expr_t base;
  string name;
  function<expr_t*(expr_t*, expr_env_t*)> f;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_special_form_t {
  expr_special_form_t(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f);

  expr_t base;
  string name;
  function<expr_t*(expr_t*, expr_env_t*)> f;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_macro_t {
  expr_macro_t(const string& name, const function<expr_t*(expr_t*, expr_env_t*)>& f);

  expr_t base;
  string name;
  function<expr_t*(expr_t*, expr_env_t*)> f;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_compound_proc_t {
  expr_compound_proc_t(expr_t* params, expr_t* body);

  expr_t base;
  expr_t* params;
  expr_t* body;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_cons_t {
  expr_cons_t(expr_t* first, expr_t* second);

  expr_t base;
  expr_t* first;
  expr_t* second;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_istream_t {
  expr_istream_t(istream& is);
  expr_istream_t(unique_ptr<istream> is);

  expr_t base;
  unique_ptr<istream, function<void(istream*)>> is;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_ostream_t {
  expr_ostream_t(ostream& os);
  expr_ostream_t(unique_ptr<ostream> os);

  expr_t base;
  unique_ptr<ostream, function<void(ostream*)>> os;

  string to_string();
  void print(ostream& os = cout, const string& prefix = "", bool is_last = true);
};

struct expr_exception_t : public exception {
  expr_exception_t(const string& message, expr_t* expr);

  expr_t* expr;
  string message;

  const char* what() const noexcept override;
};

bool is_eof(expr_t* expr);

bool is_void(expr_t* expr);

bool is_nil(expr_t* expr);

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

bool is_primitive_proc(expr_t* expr);

bool is_special_form(expr_t* expr);

bool is_macro(expr_t* expr);

bool is_compound_proc(expr_t* expr);
expr_t* get_compound_proc_params(expr_t* expr);
expr_t* get_compound_proc_body(expr_t* expr);

bool is_istream(expr_t* expr);
istream& get_istream(expr_t* expr);

bool is_ostream(expr_t* expr);
ostream& get_ostream(expr_t* expr);

#endif // EXPR_H

