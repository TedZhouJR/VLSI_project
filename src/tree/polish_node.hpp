//  polish_node.hpp
//  sa

#ifndef polish_node_hpp
#define polish_node_hpp

#include <cstdint>
#include <ostream>

namespace polish {

    class basic_polish_node {
    public:
        enum class CombineType {
            VERTICAL, HORIZONTAL, LEAF
        };
        using combine_type = CombineType;
        using dimension_type = std::int32_t;    // YAL都是整型, 故而作此改动
        using size_type = std::uint32_t;

        combine_type type;	//*(左右结合) or +(上下结合)
        dimension_type height;
        dimension_type width;

        basic_polish_node(combine_type type_in, dimension_type height_in,
            dimension_type width_in) : type(type_in), height(height_in), 
            width(width_in) {}

        static combine_type invert_combine_type(combine_type c) {
            return c == combine_type::LEAF ? 
                combine_type::LEAF : c == combine_type::HORIZONTAL ?
                combine_type::VERTICAL : combine_type::HORIZONTAL;
        }
    };

    std::ostream &operator<<(std::ostream &os, const basic_polish_node &n);

    class polish_node : public basic_polish_node {
        using base = basic_polish_node;
        using self = polish_node;

    public:
        using combine_type = typename base::combine_type;
        using dimension_type = typename base::dimension_type;
        using size_type = typename base::size_type;

        polish_node* lc;	//left child
        polish_node* rc;	//right child
        polish_node* parent;	//parent node
        size_type size;     //subtree size

        polish_node(combine_type type_in, dimension_type height_in,
            dimension_type width_in) : base(type_in, height_in, width_in),
            lc(nullptr), rc(nullptr), parent(nullptr), size(1) {}

        static polish_node make_header() {
            return polish_node(combine_type::LEAF, 0, 0);
        }

        void count_area();

        bool check_area() const;

        void count_size() noexcept {
            if (lc)
                size = lc->size + rc->size + 1;
        }

        bool check_size() const noexcept {
            if (is_leaf())
                return size == 1;
            else
                return size == lc->size + rc->size + 1;
        }

        bool is_leaf() const noexcept {
            return !lc;
        }

        bool is_header() const noexcept {
            return !parent;
        }

        const self *prev() const;

        self *prev() {
            return const_cast<self *>(const_cast<const self *>(this)->prev());
        }

        const self *next() const;

        self *next() {
            return const_cast<self *>(const_cast<const self *>(this)->next());
        }

        // For debug.
        std::ostream &print_tree(std::ostream &os, int offset, 
            int ident, char fill = ' ', bool print_size = false) const;
    };

}   // polish

#endif /* polish_node_hpp */
