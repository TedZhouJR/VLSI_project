// rect.h: struct Rect. Mainly for compatibility with old code.
// Modified by LYL (Aureliano Lee)

#pragma once
#include "xseqpair.h"
#include <ostream>
#include "point.h"

namespace seqpair {

    struct Rect {
        enum class format_policy { delim, no_delim };

        struct formatted {
            formatted(const Rect &rect, 
                typename Rect::format_policy format_policy) :
                rect(rect), format_policy(format_policy) { } 
            const Rect &rect;
            typename Rect::format_policy format_policy;
        };

        Rect() : Rect(0, 0, 0, 0) {}
        Rect(Point left_bottom, int width, int height) : 
            Rect(left_bottom.x, left_bottom.y, width, height) { }
        Rect(int x, int y, int width, int height) : 
            pos(x, y), width(width), height(height) { }
        
        int left() const noexcept { return pos.x; }
        int right() const noexcept { return pos.x + width; }
        int bottom() const noexcept { return pos.y; }
        int top() const noexcept { return pos.y + height; }
        double hcenter() const noexcept { return pos.x + width / 2.0; }
        double vcenter() const noexcept { return pos.y + height / 2.0; }

        formatted format(format_policy policy = format_policy::delim) const {
            return formatted(*this, policy);
        }
           
        int width, height;
        Point pos;   // left-bottom
    };

    bool intersects(const Rect &r0, const Rect &r1) noexcept;

    std::istream &operator>>(std::istream &in, Rect &r);
    std::ostream &operator<<(std::ostream &out, const Rect &r);
    std::ostream &operator<<(std::ostream &out, const typename Rect::formatted &r);
}
