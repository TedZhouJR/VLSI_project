// point.h: struct Point. Mainly for compatibility with old code.
// Modified by LYL (Aureliano Lee)

#pragma once
#include <tuple>
#include <utility>

namespace seqpair {
    struct Point {
        Point() : Point(0, 0) { }
        Point(int x, int y) : x(x), y(y) { }

        template<typename Pair>
        explicit Point(const Pair &p) : 
            Point(std::get<0>(p), std::get<1>(p)) { }
        
        template<typename Ty0, typename Ty1>
        explicit operator std::pair<Ty0, Ty1>() const {
            return { x, y };
        }

        int x, y;
    };
}

