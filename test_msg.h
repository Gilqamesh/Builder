#ifndef TEST_MSG_H
# define TEST_MSG_H

# include "ipc_cpp/mem.h"
# include "ipc_cpp/msg.h"

enum class custom_message_t : uint16_t {
    MESSAGE_TEXT,
    MESSAGE_RPC
};

typedef void (*simple_fn_signature_t)(int);

template <typename signature_t>
struct rpc_t {
    signature_t m_fn;
    std::vector<uint8_t> m_params;
};

struct shared_t {
    shared_t(abs_ptr_t<shared_memory_t> shared_memory):
        m_messages(shared_memory) {
    }

    message_queue_t<custom_message_t> m_messages;

    simple_fn_signature_t fn;
};

#endif // TEST_MSG_H
