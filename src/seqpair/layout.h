// layout.h: class LayoutBase and class Layout. 
// Rewritten by LYL (Aureliano Lee)

#pragma once
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>
#include "rect.h"

namespace seqpair {    

    // LayoutBase only stores the bottom-left positions of components. 
    // Widths and heights are shared. 
    template<typename Alloc = std::allocator<void> >
    class LayoutBase {
    public:
        using allocator_type = Alloc;
        using int_alloc_t = typename std::allocator_traits<allocator_type>::
            template rebind_alloc<int>;
        using pos_vector_t = std::vector<int, int_alloc_t>;

        LayoutBase() : LayoutBase(0) { }

        explicit LayoutBase(std::size_t sz) : 
            LayoutBase(sz, allocator_type()) { }

        LayoutBase(std::size_t sz, const allocator_type &alloc) : 
            _x(sz, alloc), _y(sz, alloc) { }

        template<typename Vctr0, typename Vctr1>
        std::pair<int, int> get_area(const Vctr0 &widths,
            const Vctr1 &heights) const {
            int lX = INT_MAX, rX = INT_MIN, bY = INT_MAX, tY = INT_MIN;
            int x, y, w, h;
            BOOST_FOREACH(boost::tie(x, y, w, h),
                boost::combine(_x, _y, widths, heights)) {
                Rect r(x, y, w, h);
                if (lX > r.left())
                    lX = r.left();
                if (rX < r.right())
                    rX = r.right();
                if (bY > r.bottom())
                    bY = r.bottom();
                if (tY < r.top())
                    tY = r.top();
            }
            return { rX - lX, tY - bY };
        }

        // Note: not used.
        template<typename Cont0, typename Cont1, typename OutIt0, 
            typename OutIt1>
        std::pair<OutIt0, OutIt1> get_constraints(const Cont0 &widths,
            const Cont1 &heights, OutIt0 hcons, OutIt1 vcons) const {
            using namespace std;
            for (std::size_t i = 0; i != size(); ++i) {
                Rect ri(_x[i], _y[i], widths[i], heights[i]);
                int ix1 = ri.left();
                int ix2 = ri.right();
                int iy1 = ri.bottom();
                int iy2 = ri.top();
                for (std::size_t j = 0; j != size(); ++j) {
                    Rect rj(_x[j], _y[j], widths[j], heights[j]);
                    int jx1 = rj.left();
                    int jx2 = rj.right();
                    int jy1 = rj.bottom();
                    int jy2 = rj.top();
                    if ((ix2 <= jx1) && !(iy2 < jy1 || jy2 < iy1))
                        *hcons++ = make_pair(i, j);
                    else if ((jx2 <= ix1) && !(iy2 < jy1 || jy2 < iy1))
                        *hcons++ = make_pair(j, i);
                    else if ((iy2 <= jy1) && !(ix2 < jx1 || jx2 < ix1))
                        *vcons++ = make_pair(i, j);
                    else if ((jy2 <= iy1) && !(ix2 < jx1 || jx2 < ix1))
                        *vcons++ = make_pair(j, i);
                }
            }
            return { hcons, vcons };
        }

        auto size() const noexcept {
            return _x.size();
        }

        bool empty() const noexcept {
            return _x.empty();
        }

        // Const view
        const pos_vector_t &x() const noexcept {
            return _x;
        }

        // Const view
        const pos_vector_t &y() const noexcept {
            return _y;
        }

        void set_x(std::size_t k, int x) {
            _x[k] = x;
        }

        void set_y(std::size_t k, int y) {
            _y[k] = y;
        }

        auto x_begin() {
            return _x.begin();
        }

        auto x_begin() const {
            return _x.begin();
        }

        auto x_end() {
            return _x.end();
        }

        auto x_end() const {
            return _x.end();
        }

        auto y_begin() {
            return _y.begin();
        }

        auto y_begin() const {
            return _y.begin();
        }

        auto y_end() {
            return _y.end();
        }

        auto y_end() const {
            return _y.end();
        }

    protected:
        pos_vector_t _x, _y;
    };


    // General layout.
    template<typename Alloc = std::allocator<void>>
    class Layout : public LayoutBase<Alloc> {
        using self_t = Layout<Alloc>;
        using base_t = LayoutBase<Alloc>;

    protected:
        using typename base_t::int_alloc_t;

    public:
        using typename base_t::allocator_type;
        
        using typename base_t::pos_vector_t;
        using size_vector_t = std::vector<int, int_alloc_t>;
        using format_policy = typename Rect::format_policy;

        struct formatted {
            formatted(const Layout<Alloc> &layout, format_policy policy) :
                layout(layout), policy(policy) { }
            const Layout<Alloc> &layout;
            format_policy policy;
        };

        Layout() : Layout(0, allocator_type()) { }

        explicit Layout(const allocator_type &alloc) : Layout(0, alloc) { }

        Layout(std::size_t sz, const allocator_type &alloc) : 
            base_t(sz, alloc), _widths(alloc), _heights(alloc) { }

        // Each iterator should point to a pair-like structure of width and height
        // (must support std::get).
        template<typename InIt>
        Layout(InIt first_component, InIt last_component, 
            const allocator_type &alloc = allocator_type()) : 
            Layout(alloc) {
            for (auto i = first_component; i != last_component; ++i)
                push(std::get<0>(*i), std::get<1>(*i));
        }

        // Pushes fixed-size component.
        void push(int width, int height) {
            _widths.push_back(width);
            _heights.push_back(height);
            base_t::_x.emplace_back();
            base_t::_y.emplace_back();
        }

        // Pushes fixed-size component.
        template<typename Pair>
        void push(const Pair &p) {
            push(std::get<0>(p), std::get<1>(p));
        }

        void clear() {
            _widths.clear();
            _heights.clear();
            base_t::_x.clear();
            base_t::_y.clear();
        }

        const size_vector_t &widths() const noexcept {
            return _widths;
        }

        const size_vector_t &heights() const noexcept {
            return _heights;
        }

        auto widths_begin() {
            return _widths.begin();
        }

        auto widths_begin() const {
            return _widths.begin();
        }

        auto widths_end() {
            return _widths.end();
        }

        auto widths_end() const {
            return _widths.end();
        }

        auto heights_begin() {
            return _heights.begin();
        }

        auto heights_begin() const {
            return _heights.begin();
        }

        auto heights_end() {
            return _heights.end();
        }

        auto heights_end() const {
            return _heights.end();
        }

        // Read-only Rect view.
        Rect rect(std::size_t k) const {
            return Rect(base_t::_x[k], base_t::_y[k], _widths[k], _heights[k]);
        }

        std::pair<int, int> get_area() const noexcept {
            return base_t::get_area(_widths, _heights);
        }

        template<typename OutIt0, typename OutIt1>
        std::pair<OutIt0, OutIt1> get_constraints(OutIt0 hcons, OutIt1 vcons) {
            return base_t::get_constraints(_widths, _heights, hcons, vcons);
        }

        formatted format(format_policy policy = format_policy::delim) const {
            return formatted(*this, policy);
        }

        int sum_conponent_areas() const {
            return std::inner_product(widths_begin(), widths_end(), heights_begin(), 0);
        }

        friend std::istream &operator>>(std::istream &in, Layout<Alloc> &layout) {
            return layout._read(in);
        }

        friend std::ostream &operator<<(std::ostream &out, const Layout<Alloc> &layout) {
            return layout._print(out, format_policy::delim);
        }

        friend std::ostream &operator<<(std::ostream &out, const formatted &formatted) {
            return formatted.layout._print(out, formatted.policy);
        }

    protected:
        std::istream &_read(std::istream &in) {
            Rect r;
            while (in >> r) {
                base_t::_x.push_back(r.pos.x);
                base_t::_y.push_back(r.pos.y);
                _widths.push_back(r.width);
                _heights.push_back(r.height);
            }
            return in;
        }

        std::ostream &_print(std::ostream &out, format_policy policy) const {
            int x, y, w, h;
            BOOST_FOREACH(boost::tie(x, y, w, h),
                boost::combine(base_t::_x, base_t::_y, _widths, _heights))
                out << Rect(x, y, w, h).format(policy) << "\n";
            return out;
        }

        size_vector_t _widths, _heights;
    };


    namespace detail {
        template<typename Alloc0, typename Alloc1>
        void unguarded_copy_layout_positions(const LayoutBase<Alloc0> &src, 
            LayoutBase<Alloc1> &dest) {
            assert(src.size() == dest.size());
            auto sz = src.size();
            std::copy(src.x().data(), src.x().data() + sz, std::addressof(*dest.x_begin()));
            std::copy(src.y().data(), src.y().data() + sz, std::addressof(*dest.y_begin()));
        }

        template<typename Alloc0, typename Alloc1>
        void unguarded_copy_layout_sizes(const Layout<Alloc0> &src, Layout<Alloc1> &dest) {
            assert(src.size() == dest.size());
            auto sz = src.size();
            std::copy(src.widths().data(), src.widths().data() + sz, 
                std::addressof(*dest.widths_begin()));
            std::copy(src.heights().data(), src.heights().data() + sz, 
                std::addressof(*dest.heights_begin()));
        }

        template<typename Alloc0, typename Alloc1>
        void unguarded_copy_layout(const Layout<Alloc0> &src, Layout<Alloc1> &dest) {
            unguarded_copy_layout_sizes(src, dest);
            unguarded_copy_layout_positions(src, dest);
        }
    }
}

