#include "data.h"

#include <queue>
#include <memory>
#include <cassert>

data_t::data_t(type_t* type): type(type) {
    if (type->is_abstract()) {
        throw runtime_error("cannot make instance of abstract type: '" + type->name + "'");
    }
}

bool data_t::convert(data_t& result) const {
    type_t* from = type;
    type_t* to = result.type;
    if (to == from) {
        return 0;
    }

    unordered_set<type_t*> visited;
    visited.insert(from);

    struct converter_t {
        type_t* to;
        function<bool(const data_t&, data_t&)> convert_to_fn;
        shared_ptr<converter_t> prev;
    };

    function<bool(shared_ptr<converter_t>, const data_t&, data_t&)> convert_impl;
    convert_impl = [&convert_impl](shared_ptr<converter_t> converter, const data_t& from, data_t& result) {
        assert(converter);
        if (!converter->prev) {
            if (converter->convert_to_fn(from, result)) {
                return true;
            } else {
                return false;
            }
        }

        data_t subresult(converter->to);
        if (convert_impl(converter->prev, from, subresult)) {
            if (converter->convert_to_fn(subresult, result)) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    };

    queue<shared_ptr<converter_t>> q;
    for (const auto& p : from->convert_to_fns) {
        q.push(make_shared<converter_t>(converter_t{ p.first, p.second, 0 }));
    }
    while (!q.empty()) {
        shared_ptr<converter_t> converter = q.front();
        q.pop();
        auto r = visited.insert(converter->to);
        assert(r.second);
        if (converter->to == to) {
            assert(converter->to == to);
            if (convert_impl(converter, *this, result)) {
                return true;
            }
        }
        for (const auto& p : converter->to->convert_to_fns) {
            auto it = visited.find(p.first);
            if (it == visited.end()) {
                shared_ptr<converter_t> next_converter = make_shared<converter_t>(converter_t{ p.first, p.second, converter });
                if (next_converter->to == to) {
                    if (convert_impl(next_converter, *this, result)) {
                        return true;
                    }
                } else {
                    q.push(next_converter);
                }
            }
        }
    }

    return false;
}

