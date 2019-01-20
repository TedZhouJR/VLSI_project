#pragma once

#include <algorithm>
#include <vector>

#include "polish_node.hpp"

namespace polish {
    namespace detail {
        inline bool overlap(meta_polish_node::dimension_type lo0, 
            meta_polish_node::dimension_type hi0,
            meta_polish_node::dimension_type lo1, 
            meta_polish_node::dimension_type hi1) noexcept {
            return (lo0 < hi1) ^ (lo1 >= hi0);
        }

        // Whether two (x, y, w, h) tuples intersect.
        template<typename Tuple>
        bool overlap(const Tuple &a,
            const Tuple &b) noexcept {
            return overlap(get<0>(a), get<0>(a) + get<2>(a),
                get<0>(b), get<0>(b) + get<2>(b))
                && overlap(get<1>(a), get<1>(a) + get<3>(a),
                    get<1>(b), get<1>(b) + get<3>(b));
        }
    }

    // Whether a range of (x, y, w, h) tuples overlap.
    template<typename InIt>
    bool overlap(InIt first, InIt last) {
        using tuple_type = typename std::iterator_traits<InIt>::value_type;
        std::vector<tuple_type> v(first, last);
        std::sort(v.begin(), v.end(), 
            [](const tuple_type &t1, const tuple_type &t2) {
            return std::get<0>(t1) < std::get<0>(t2);
        });
        return std::adjacent_find(v.cbegin(), v.cend(), 
            [](const tuple_type &t1, const tuple_type &t2) {
            return detail::overlap(t1, t2);
        }) != v.cend();
    }

}