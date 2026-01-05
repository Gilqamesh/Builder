#ifndef BUILDER_PROJECT_BUILDER_BUILDER_H
# define BUILDER_PROJECT_BUILDER_BUILDER_H

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

typedef enum library_type_t {
    LIBRARY_TYPE_STATIC,
    LIBRARY_TYPE_SHARED,

    __LIBRARY_TYPE_SIZE
} library_type_t;

typedef struct builder_t builder_t;
BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type);
BUILDER_EXTERN void builder__import_libraries(const builder_t* builder);

#endif // BUILDER_PROJECT_BUILDER_BUILDER_H
