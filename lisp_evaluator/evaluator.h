#ifndef EVALUATOR_H
# define EVALUTOR_H

# include "parser.h"

struct evaluator_t {
  evaluator_t();

  evaluate(expr_t* expr);
};

#endif // EVALUATOR_H

