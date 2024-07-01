#include "program.h"

namespace program {

template <typename program_t, typename... Args>
offset_ptr_t<program_t> context_t::malloc_program_named(const std::string& program_name, Args&&... args) {
    offset_ptr_t<program_t> program = m_shared_memory->malloc_named<program_t>(program_name, this, std::forward<Args>(args)...);
    m_shared_namespace->m_programs.push_back(program);
    return program;
}

template <typename program_t, typename... Args>
offset_ptr_t<program_t> context_t::malloc_program(Args&&... args) {
    offset_ptr_t<program_t> program = m_shared_memory->malloc<program_t>(this, std::forward<Args>(args)...);
    m_shared_namespace->m_programs.push_back(program);
    return program;
}

template <typename program_t>
offset_ptr_t<program_t> context_t::find_named_program(const std::string& program_name) {
    return m_shared_memory->find_named<program_t>(program_name);
}

} // namespace program
