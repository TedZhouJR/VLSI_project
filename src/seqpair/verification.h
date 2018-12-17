// verification.h: tools for verification.
// Author: LYL (Aureliano Lee)

#pragma once
#include "xseqpair.h"
#include <algorithm>
#include <numeric>
#include <random>
#include "layout.h"

namespace seqpair {
    namespace verification {
        namespace detail {
            // Recursively divides the rectangle.
            template<typename Alloc, typename Eng>
            size_t make_square_layout_impl(Layout<Alloc> &layout, size_t sz,
                int width, int height, Eng &&eng) {
                using std::swap;
                if (width == 0 || height == 0 || sz == 0)
                    return 0;
                if ((width == 1 && height == 1) || sz == 1) {
                    layout.push(width, height);
                    return 1;
                }
                if (width == 1 || std::bernoulli_distribution()(std::forward<Eng>(eng)))
                    swap(width, height);
                auto k = std::uniform_int_distribution<>(1, width - 1)(std::forward<Eng>(eng));
                auto cnt0 = make_square_layout_impl(layout, sz >> 1, k,
                    height, std::forward<Eng>(eng));
                auto cnt1 = make_square_layout_impl(layout, sz - cnt0,
                    width - k, height, std::forward<Eng>(eng));
                return cnt0 + cnt1;
            }
        }   // detail

        // Constructs a layout which in optimial can be exactly packed into a rectangle 
        // (width * height).
        template<typename Eng, typename Alloc = std::allocator<void>>
        Layout<std::decay_t<Alloc>> make_square_layout(size_t sz,
            int width, int height, Eng &&eng, Alloc &&alloc = Alloc()) {
            Layout<std::decay_t<Alloc>> layout(std::forward<Alloc>(alloc));
            detail::make_square_layout_impl(layout, sz, width, height, 
                std::forward<Eng>(eng));
        }

        // Makes a layout using uniform distribution.
        template<typename Eng, typename Alloc = std::allocator<void>>
        Layout<std::decay_t<Alloc>> make_random_layout(size_t sz, int min_len, int max_len,
            Eng &&eng, Alloc &&alloc = Alloc()) {
            Layout<std::decay_t<Alloc>> layout(std::forward<Alloc>(alloc));
            std::uniform_int_distribution<int> rand_int(min_len, max_len);
            while (sz--) {
                layout.push(rand_int(eng), rand_int(eng));
            }
            return layout;
        }

        // Checks for overlap.
        template<typename Alloc>
        bool has_intersection(const Layout<Alloc> &layout) noexcept {
            for (size_t i = 0; i != layout.size(); ++i)
                for (size_t j = i + 1; j != layout.size(); ++j)
                    if (intersects(layout.rect(i), layout.rect(j)))
                        return true;
            return false;
        }

        // Tries to scatter [0, n) to count pairs, and writes them to dest.
        template<typename Ty, typename OutIt, typename Eng>
        std::pair<OutIt, bool> random_scatter_to_pairs(Ty n, size_t count, OutIt dest, Eng &&eng) {
            using namespace std;
            vector<Ty> vctr(n);
            iota(begin(vctr), end(vctr), static_cast<Ty>(0));
            for (; count--; ) {
                if (vctr.size() < 2)
                    return { dest, false };
                swap(vctr[uniform_int_distribution<size_t>(0, vctr.size() - 1)(
                    std::forward<Eng>(eng))], vctr.back());
                auto x = vctr.back();
                vctr.pop_back();
                swap(vctr[uniform_int_distribution<size_t>(0, vctr.size() - 1)(
                    std::forward<Eng>(eng))], vctr.back());
                auto y = vctr.back();
                vctr.pop_back();
                *dest++ = { x, y };
            }
            return { dest, true };
        }
    }   // verification
}   // seqpair
