#ifndef EXAMPLE_H
# define EXAMPLE_H

# if defined(__cplusplus)
#  define PUBLIC_API extern "C"
# else
#  define PUBLIC_API
# endif

PUBLIC_API void print_next(float a);
PUBLIC_API void print_prev(float a);

#endif // EXAMPLE_H
