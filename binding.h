#ifndef KERNEL_BINDING_H
# define KERNEL_BINDING_H

# include <string>

namespace kernel {

namespace binding {

class binding_t {
public:
    binding_t(std::string key, std::string value);

    const std::string& key() const;
    const std::string& value() const;

private:
    std::string m_key;
    std::string m_value;
};

} // namespace binding

} // namespace kernel

#endif // KERNEL_BINDING_H
