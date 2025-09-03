#ifndef NODES_H
# define NODES_H

# include "core/node.h"

/**
 * 0: condition (bool)
 * 1: consequence (int)
 * 2: alternative (int)
 * 3: output (int)
 * 4: output (int)
*/
class if_node_t : public node_t {
public:
    void call(port_index_t caller_port_index) override;
};

/**
 * 0: a (int)
 * 1: b (int)
 * 2: a - b (int)
*/
class sub_t : public node_t {
public:
    void call(port_index_t caller_port_index) override;
};

/**
 * 0: a (int)
 * 1: b (int)
 * 2: a * b (int)
*/
class mul_node_t : public node_t {
public:
    void call(port_index_t caller_port_index) override;
};

/**
 * 0: in (int)
 * 1: out (bool)
*/
class is_zero_t : public node_t {
public:
    void call(port_index_t caller_port_index) override;
};

/**
 * 0: out (int)
*/
class int_node_t : public node_t {
public:
    int_node_t(int val = 1);

    void call(port_index_t caller_port_index) override;

private:
    int m_val;
};

/**
 * 0: value (int)
 * [1]: out (ostream*)
*/
class logger_node_t : public node_t {
public:
    void call(port_index_t caller_port_index) override;
};

/**
 * 0-n: (any)
*/
class pin_node_t : public node_t {
public:
    /**
     * Broadcasts data received on all ports except to `caller_port_index`.
    */
    void call(port_index_t caller_port_index) override;
};

#endif // NODES_H
