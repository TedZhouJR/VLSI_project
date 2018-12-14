//  polish_node.hpp

#ifndef polish_node_hpp
#define polish_node_hpp

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ostream>
#include <vector>

namespace polish {

    // Common base for node value interfaces.
    class meta_polish_node {
    public:
        enum class combine_type {
            VERTICAL, HORIZONTAL, LEAF
        };
        using dimension_type = std::int32_t;    // YAL都是整型, 故而作此改动

        // Not used by basic_polish_node
        using coord_type = std::pair<dimension_type, dimension_type>;

        combine_type type;	//*(左右结合) or +(上下结合)

        explicit meta_polish_node(combine_type type_in) : 
            type(type_in) {}

        static combine_type invert_combine_type(combine_type c) {
            return c == combine_type::LEAF ?
                combine_type::LEAF : c == combine_type::HORIZONTAL ?
                combine_type::VERTICAL : combine_type::HORIZONTAL;
        }
    };

    std::ostream & operator<<(std::ostream &os, const meta_polish_node &n);

    // Public interface for tree of hard modules.
    class basic_polish_node : public meta_polish_node {
        using self = basic_polish_node;
        using base = meta_polish_node;

    public:
        using typename base::coord_type;
        using typename base::dimension_type;

        dimension_type height;
        dimension_type width;

        basic_polish_node(combine_type type_in, dimension_type height_in,
            dimension_type width_in) : base(type_in), height(height_in), 
            width(width_in) {}

        static self make_header() {
            return self(combine_type::LEAF, 0, 0);
        }

        void count_area(const self &lc, const self &rc) noexcept {
            assert(this->type != combine_type::LEAF);
            if (this->type == combine_type::VERTICAL) {	//上下结合
                this->height = lc.height + rc.height;
                this->width = std::max(lc.width, rc.width);
            } else {	//左右结合
                this->height = std::max(lc.height, rc.height);
                this->width = lc.width + rc.width;
            }
        }

        bool check_area(const self &lc, const self &rc) const noexcept {
            auto tmp = *this;
            return width == tmp.width && height == tmp.height;
        }
    };

    std::ostream & operator<<(std::ostream &os, const basic_polish_node &n);

    // Public interface for tree of soft modules.
    // Alloc allocates coord_type.
    // Its points field (i.e., curve function) is as follows:
    //    |
    //    |
    //    *----
    //         |
    //         *-----
    //               |
    //               *---------------------------
    template<typename Alloc = std::allocator<typename meta_polish_node::coord_type>>
    class basic_vectorized_polish_node : public meta_polish_node {
        using self = basic_vectorized_polish_node;
        using base = meta_polish_node;

    public:
        using allocator_type = Alloc;
        using typename base::coord_type;
        using typename base::dimension_type;

        basic_vectorized_polish_node(combine_type type, 
            const allocator_type &alloc) : 
            base(type), points(alloc) {}

        static self make_header() {
            return self();
        }

        void count_area(const self &lc, const self &rc) {
            assert(this->type != combine_type::LEAF);
            assert(!lc.points.empty() && !rc.points.empty());
            points.clear();
            if (this->type == combine_type::VERTICAL) {
                auto i = lc.points.begin(), j = rc.points.begin();
                constexpr dimension_type inf 
                    = std::numeric_limits<dimension_type>::max();
                dimension_type yi = inf, yj = inf;
                while (i != lc.points.end() && j != rc.points.end()) {
                    if (i->first < j->first) {
                        if (yj != inf)
                            points.emplace_back(i->first, i->second + yj);
                        yi = i->second;
                        ++i;
                    } else if (i->first > j->first) {
                        if (yi != inf)
                            points.emplace_back(j->first, j->second + yi);
                        yj = j->second;
                        ++j;
                    } else {
                        points.emplace_back(i->first, i->second + j->second);
                        yi = i->second;
                        yj = j->second;
                        ++i; ++j;
                    }
                }
                while (i != lc.points.end()) {
                    points.emplace_back(i->first, i->second + yj);
                    ++i;
                }
                while (j != rc.points.end()) {
                    points.emplace_back(j->first, j->second + yi);
                    ++j;
                }
            } else {
                auto i = lc.points.begin(), j = rc.points.begin();
                while (i != lc.points.end() && j != rc.points.end()) {
                    if (i->second > j->second) {
                        points.emplace_back(i->first + j->first, i->second);
                        ++i;
                    } else if (i->second < j->second) {
                        points.emplace_back(i->first + j->first, j->second);
                        ++j;
                    } else {
                        points.emplace_back(i->first + j->first, i->second);
                        ++i; ++j;
                    }
                }
            }
        }

        bool check_area(const self &lc, const self &rc) {
            auto tmp = *this;
            tmp.count_area(lc, rc);
            return points == tmp.points;
        }

        std::vector<coord_type, allocator_type> points;

    private:
        basic_vectorized_polish_node() : base(combine_type::LEAF) {}
    };

    template<typename Alloc>
    std::ostream & operator<<(std::ostream &os, const basic_vectorized_polish_node<Alloc> &n) {
        if (n.type != meta_polish_node::combine_type::LEAF)
            return os << static_cast<const meta_polish_node &>(n);
        for (auto i = n.points.begin(); i != n.points.end(); ++i) {
            os << "(" << i->first << "," << i->second << ")";
            if (i != n.points.end())
                os << " ";
        }
        return os;
    }

    // Node base storing structural information.
    class tree_node_base {
        using self = tree_node_base;

    public:
        using size_type = std::uint32_t;

        tree_node_base() : lc_(nullptr), rc_(nullptr), parent_(nullptr), size(1) {}
        bool is_leaf() const noexcept { return !lc_; }
        bool is_header() const noexcept { return !parent_; }
        const self *prev() const noexcept;
        const self *next() const noexcept;

        void count_size() noexcept {
            if (!is_leaf())
                size = lc_->size + rc_->size + 1;
        }

        // For debug.
        bool check_size() const noexcept {
            if (is_leaf())
                return size == 1;
            else
                return size == lc_->size + rc_->size + 1;
        }

        self *lc_;	        //left child
        self *rc_;	        //right child
        self *parent_;	    //parent node
        size_type size;     //subtree size
    };

    // Actual node stored in tree.
    template<typename BaseNode>
    class tree_node : public tree_node_base, public BaseNode {
        // NOTE: tree_node_base goes first, because we need to mix
        // tree_node_base * and tree_node *.
        using self = tree_node;
        using base = BaseNode;

    public:
        using typename base::coord_type;
        using typename base::dimension_type;
        using typename tree_node_base::size_type;

        // Forward all arguments to base types.
        template<typename... Types>
        tree_node(Types &&...args) : tree_node_base(), 
            base(std::forward<Types>(args)...) {}

        static self make_header() {
            return self();
        }

        const self *lc() const noexcept {
            return static_cast<const self *>(this->lc_);
        }

        self *&lc() noexcept {
            return reinterpret_cast<self *&>(this->lc_);
        }

        const self *rc() const noexcept {
            return static_cast<const self *>(this->rc_);
        }

        self *&rc() noexcept {
            return reinterpret_cast<self *&>(this->rc_);
        }

        const self *parent() const noexcept {
            return static_cast<const self *>(this->parent_);
        }

        self *&parent() noexcept {
            return reinterpret_cast<self *&>(this->parent_);
        }

        void count_area() {
            if (!is_leaf())
                base::count_area(*lc(), *rc());
        }

        // For debug.
        bool check_area() const {
            return is_leaf() || base::check_area(*lc(), *rc());
        }

        const self *prev() const noexcept {
            return static_cast<const self *>(tree_node_base::prev());
        }

        self *prev() noexcept {
            return const_cast<self *>(const_cast<const self *>(this)->prev());
        }

        const self *next() const noexcept {
            return static_cast<const self *>(tree_node_base::next());
        }

        self *next() noexcept {
            return const_cast<self *>(const_cast<const self *>(this)->next());
        }

        // For debug.
        std::ostream &print_tree(std::ostream &os, int offset,
            int ident, char fill = ' ') const {
            if (rc())
                rc()->print_tree(os, offset + ident, ident, fill);
            for (int i = 0; i < offset; ++i)
                os << fill;
            os << static_cast<const base &>(*this) << "\n";
            if (lc())
                lc()->print_tree(os, offset + ident, ident, fill);
            return os;
        }

    private:
        tree_node() : tree_node_base(), base(base::make_header()) {}
    };

    using polish_node = tree_node<basic_polish_node>;

    template<typename Alloc>
    using vectorized_polish_node = tree_node<basic_vectorized_polish_node<Alloc>>;

}   // polish

#endif /* polish_node_hpp */
