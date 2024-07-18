#include "program.h"

namespace program {

template <typename signature_t>
signature_t fn_signature_derived_t<signature_t>::add(ipc_mem::offset_ptr_t<base_t> program, const std::string& name, const signature_t& fn) {
    assert(!find(program, name));
    std::string program_type;
    program->m_program_type.read([&program_type](auto& string) {
        program_type = string;
    });
    auto it_name_to_fn_ptr = m_program_type_to_name_to_fn_ptr.find(program->m_program_type);
    if (it_name_to_fn_ptr == m_program_type_to_name_to_fn_ptr.end()) {
        auto emplace_result = m_program_type_to_name_to_fn_ptr.emplace(
            program_type,
            malloc<name_to_fn_ptr_t>()
        );
        assert(emplace_result.second);
        it_name_to_fn_ptr = emplace_result.first;
        assert(it_name_to_fn_ptr != m_program_type_to_name_to_fn_ptr.end());
    }

    auto emplace_result = it_name_to_fn_ptr->second->emplace(name, malloc<signature_t>(fn));
    assert(emplace_result.second);
    return *emplace_result.first->second;
}

template <typename signature_t>
signature_t fn_signature_derived_t<signature_t>::find(ipc_mem::offset_ptr_t<base_t> program, const std::string& name) {
    auto it_program_type_to_name_to_fn_ptr = m_program_type_to_name_to_fn_ptr.find(program->m_program_type);
    if (it_program_type_to_name_to_fn_ptr == m_program_type_to_name_to_fn_ptr.end()) {
        return 0;
    }
    
    auto it_fn_ptr = it_program_type_to_name_to_fn_ptr->second->find(name);
    if (it_fn_ptr == it_program_type_to_name_to_fn_ptr->second->end()) {
        return 0;
    }

    return *it_fn_ptr->second;
}

template <typename program_t>
base_initializer_t<program_t>::base_initializer_t(
    std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
    run_fn_t run_fn,
    describe_fn_t describe_fn
): base_t(typeid(program_t).name()) {
    if (!find_fn<decltype(run_fn)>(this, "run")) {
        add_fn<decltype(run_fn)>(this, "run", run_fn);
    }
    if (!find_fn<decltype(describe_fn)>(this, "describe")) {
        add_fn<decltype(describe_fn)>(this, "describe", describe_fn);
    }

    // todo: call type specific assert check on inputs
    for (ipc_mem::offset_ptr_t<base_t> input : inputs) {
        add_input(this, input);
    }
}

template <typename signature_t>
signature_t add_fn(ipc_mem::offset_ptr_t<base_t> program, const std::string& name, const signature_t& fn) {
    if (find_fn<signature_t>(program, name)) {
        return 0;
    }

    signature_t result = 0;

    g_context->m_shared_namespace->m_fn_signatures.write([&result, &program, &name, &fn](auto& map) {
        auto it_fn_signature = map.find(std::string(typeid(signature_t).name()));
        if (it_fn_signature == map.end()) {
            map.emplace(
                std::string(typeid(signature_t).name()),
                malloc<fn_signature_derived_t<signature_t>>()
            );
            it_fn_signature = map.find(std::string(typeid(signature_t).name()));
            assert(it_fn_signature != map.end());
        }

        fn_signature_derived_t<signature_t>* signature = static_cast<fn_signature_derived_t<signature_t>*>(it_fn_signature->second.get());
        result = signature->add(program, name, fn);
    });

    return result;
}

template <typename signature_t>
signature_t find_fn(ipc_mem::offset_ptr_t<base_t> program, const std::string& name) {
    signature_t result = 0;

    g_context->m_shared_namespace->m_fn_signatures.read([&result, &program, &name](auto& map) {
        auto it_map_fn_signature_base = map.find(std::string(typeid(signature_t).name()));
        if (it_map_fn_signature_base == map.end()) {
            return 0;
        }

        fn_signature_derived_t<signature_t>* signature = static_cast<fn_signature_derived_t<signature_t>*>(it_map_fn_signature_base->second.get());
        result = signature->find(program, name);
    });

    return result;
}

template <typename signature_t, typename... Args>
auto call_fn(ipc_mem::offset_ptr_t<base_t> program, const std::string& fn_name, Args&&... args) -> decltype(std::declval<signature_t>()(std::forward<Args>(args)...)) {
    auto fn = find_fn<signature_t>(program, fn_name);
    if (!fn) {
        // note: describe is mandatory to implement for all programs, so there shouldn't be infinite loop here in case describe doesn't exist
        throw std::runtime_error("program '" + call_fn<describe_fn_t>(program, "describe", program) + "' did not register a function named " + fn_name + " under signature type " + typeid(signature_t).name());
    }

    return fn(std::forward<Args>(args)...);
}

template <typename program_t, typename... Args>
ipc_mem::offset_ptr_t<program_t> malloc_program_named(const std::string& program_name, Args&&... args) {
    ipc_mem::offset_ptr_t<program_t> program = malloc_named<program_t>(program_name, std::forward<Args>(args)...);
    g_context->m_shared_namespace->m_programs.write([&program](auto& programs_vec) {
        programs_vec.push_back(program);
    });
    return program;
}

template <typename program_t, typename... Args>
ipc_mem::offset_ptr_t<program_t> malloc_program(Args&&... args) {
    ipc_mem::offset_ptr_t<program_t> program = malloc<program_t>(std::forward<Args>(args)...);
    g_context->m_shared_namespace->m_programs.write([&program](auto& programs_vec) {
        programs_vec.push_back(program);
    });
    return program;
}

template <typename program_t>
ipc_mem::offset_ptr_t<program_t> find_named_program(const std::string& program_name) {
    return find_named<program_t>(program_name);
}

template <typename... Args>
int send(int address, const custom_message_t& type, Args&&... data) {
    int result = 0;

    g_context->m_shared_namespace->m_message_queues.write([&result, address, &type, ... data = std::forward<Args>(data)](auto& message_queues) {
        auto message_queues_it = message_queues.find(address);
        if (message_queues_it == message_queues.end()) {
            result = 1;
            return ;
        }

        auto message_queue = message_queues_it->second;

        std::cout << "[" << boost::this_process::get_id() << "] sending message to [" << address << "]" << std::endl;
        {
            message_queue->scoped_lock();
            message_queue->push_back(
                malloc<message_t<custom_message_t>>(
                    type,
                    std::forward<Args>(data)...
                )
            );
        }
    });

    return result;
}

} // namespace program
