// rect.cpp: class Rect. Mainly for compatibility with old code.

#include "rect.h"
#include <iostream>

using namespace std;

namespace seqpair {
    namespace {
        inline bool intersects(int lo0, int hi0, int lo1, int hi1) noexcept {
            return (lo0 < hi1) ^ (lo1 >= hi0);
        }
    }

    bool intersects(const Rect &r0, const Rect &r1) noexcept {
        return intersects(r0.left(), r0.right(), r1.left(), r1.right()) &&
            intersects(r0.bottom(), r0.top(), r1.bottom(), r1.top());
    }

#ifndef SEQPAIR_IO_BE_INLINE
    std::istream &operator>>(std::istream &in, Rect &r) {
        in >> r.pos.x >> r.pos.y;
        int right, top;
        in >> right >> top;
        r.width = right - r.left();
        r.height = top - r.bottom();
        return in;
    }

    std::ostream &operator<<(std::ostream &out, const Rect &r) {
        return out << "(" << r.pos.x << ", " << r.pos.y << ") - (" <<
            r.pos.x + r.width << ", " << r.pos.y + r.height << ")";
    }

    std::ostream &operator<<(std::ostream &out, const typename Rect::formatted &r) {
        const auto &rect = r.rect;
        if (r.format_policy == Rect::format_policy::delim) {
            out << rect;
        } else {
            out << rect.pos.x << " " << rect.pos.y << " " <<
                rect.pos.x + rect.width << " " << rect.pos.y + rect.height;
        }
        return out;
    }
#endif
}
