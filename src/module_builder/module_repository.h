#ifndef MODULE_REPOSITORY_H
# define MODULE_REPOSITORY_H

# include "module.h"

# include <string>
# include <unordered_map>

class module_repository_t {
public:
    void add_module(module_t* module);
    module_t* find_module(const std::string& name) const;

private:
    std::unordered_map<std::string, module_t*> m_module_by_name;
};

#endif // MODULE_REPOSITORY_H
