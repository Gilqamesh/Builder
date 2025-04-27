#ifndef CTX_H
# define CTX_H

# include "interpreter.h"
# include "memory.h"
# include "expr.h"
# include "libc.h"

struct ctx_t {
  interpreter_t interpreter;
  memory_t memory;
};

#endif // CTX_H

