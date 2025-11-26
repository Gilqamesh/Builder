#ifndef FUNCTION_ID
# define FUNCTION_ID

# include <ulid/ulid.h>

struct function_id_t {
    ulid::ULID ulid;
};

#endif // FUNCTION_ID
