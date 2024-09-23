#ifndef COMMON_H
# define COMMON_H

# include <iostream>
# include <boost/process.hpp>

# ifndef LOG
#  define LOG(module_name, x) do { \
    std::cout << boost::this_process::get_id() << " [" << module_name << "] " << x << std::endl; \
} while (0)
# endif // LOG

# if 0
#  define DEBUG_LOG_ACTIVE
# endif

# ifndef DEBUG_LOG
#  ifdef DEBUG_LOG_ACTIVE
#   define DEBUG_LOG(module_name, x) LOG(module_name, x)
#  else
#   define DEBUG_LOG(module_name, x)
#  endif // DEBUG_LOG_ACTIVE
# endif // DEBUG_LOG

#endif // COMMON_H
