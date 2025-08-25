#ifndef PROCESS_H
# define PROCESS_H

# include "libc.h"

class process_t {
public:
    process_t(std::string name, void (*f)(process_t*));
    ~process_t();

    template <typename T>
    void add_type_in(std::string name);
    template <typename T>
    void add_type_out(std::string name);

    template <typename T>
    void in(size_t index, T* obj);
    template <typename T>
    void out(size_t index, T* obj);

    void call();

private:
    std::string m_name;

    std::vector<void*> m_types_in;
    std::vector<void*> m_in;

    std::vector<void*> m_types_out;
    std::vector<void*> m_out;
};

#endif // PROCESS_H

