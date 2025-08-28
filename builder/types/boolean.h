#ifndef BOOLEAN_H
# define BOOLEAN_H

# include "core/process.h"

namespace boolean_t {

process_t* process_and();
process_t* process_or();
process_t* process_if(process_t* condition, process_t* consequence, process_t* alternative);

}

#endif // BOOLEAN_H
