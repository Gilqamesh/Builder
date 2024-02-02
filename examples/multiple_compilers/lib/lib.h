#ifndef LIB_H
# define LIB_H

# if defined(__cplusplus)
#  define PUBLIC_API extern "C"
# else
#  define PUBLIC_API
# endif

PUBLIC_API float next(float a);
PUBLIC_API float prev(float a);

#endif // LIB_H
