//  slicing_tree.hpp
//  sa

#ifndef polish_tree_hpp
#define polish_tree_hpp

#include <cassert>
#include <cstddef>
#include <vector>

#include "module.h"
#include "polish_node.hpp"

namespace polish {

    namespace expression {
        // Polish expression is a sequence of integers, each representing
        // either an operator (COMBINE_HORIZONTAL or COMBINE_VERTICAL)
        // or a non-negative module index.
        using polish_expression_type
            = std::make_signed_t<typename polish_node::size_type>;
        constexpr polish_expression_type COMBINE_HORIZONTAL = -1;
        constexpr polish_expression_type COMBINE_VERTICAL = -2;
    }

    namespace impl {

        template<typename Node>
        struct node_traits;

        template<>
        struct node_traits<basic_polish_node> {
            using value_type = basic_polish_node;
            using node_type = polish_node;
            using combine_type = typename node_type::combine_type;
            using dimension_type = typename node_type::size_type;
            using size_type = typename node_type::size_type;

            template<typename Alloc>
            static void placement_new_leaf(node_type *ptr,
                const yal::Module &m, Alloc &&) {
                ::new (ptr) node_type(meta_polish_node::combine_type::LEAF,
                    m.yspan(), m.xspan());
            }

            template<typename Alloc>
            static void placement_new_operator(node_type *ptr,
                meta_polish_node::combine_type type, Alloc &&) {
                ::new (ptr) node_type(type);
            }
        };

        template<typename Al>
        struct node_traits<basic_vectorized_polish_node<Al>> {
            using value_type = basic_vectorized_polish_node<Al>;
            using node_type = vectorized_polish_node<Al>;
            using combine_type = typename node_type::combine_type;
            using dimension_type = typename node_type::size_type;
            using size_type = typename node_type::size_type;

            template<typename Alloc>
            static void placement_new_leaf(node_type *ptr,
                const yal::Module &m, Alloc &&alloc) {
                ::new (ptr) node_type(meta_polish_node::combine_type::LEAF,
                    std::forward<Alloc>(alloc));
                auto xspan = m.xspan(), yspan = m.yspan();
                if (xspan > yspan)
                    std::swap(xspan, yspan);
                ptr->points.emplace_back(xspan, yspan);
                if (xspan != yspan)
                    ptr->points.emplace_back(yspan, xspan);
            }

            template<typename Alloc>
            static void placement_new_operator(node_type *ptr,
                meta_polish_node::combine_type type, Alloc &&alloc) {
                ::new (ptr) node_type(type, std::forward<Alloc>(alloc));
            }
        };

    }

    // Post-order bidirectional iterator.
    template<typename Traits>
    class slicing_tree_const_iterator :
        public std::iterator<std::bidirectional_iterator_tag,
        const typename Traits::value_type,
        std::make_signed_t<typename Traits::size_type>> {

        using base = std::iterator<std::bidirectional_iterator_tag,
            const typename Traits::value_type,
            std::make_signed_t<typename Traits::size_type>>;
        using self = slicing_tree_const_iterator;

    public:
        template<typename Node, typename Alloc>
        friend class slicing_tree;

        using typename base::difference_type;
        using typename base::iterator_category;
        using typename base::pointer;
        using typename base::reference;
        using typename base::value_type;

        slicing_tree_const_iterator() = default;

        reference operator*() const {
            return *ptr_;
        }

        pointer operator->() const {
            return ptr_;
        }

        self &operator++() {
            ptr_ = ptr_->next();
            return *this;
        }

        self operator++(int) {
            self ret = *this;
            ++(*this);
            return ret;
        }

        self &operator--() {
            ptr_ = ptr_->prev();
            return *this;
        }

        self operator--(int) {
            self ret = *this;
            --(*this);
            return ret;
        }

        bool operator==(const self &other) const noexcept {
            return ptr_ == other.ptr_;
        }

        bool operator!=(const self &other) const noexcept {
            return !(*this == other);
        }

    protected:
        using traits = Traits;
        using node_type = typename Traits::node_type;

        explicit slicing_tree_const_iterator(const node_type *ptr) : ptr_(ptr) {}

        const node_type *ptr_;
    };

    // Slicing tree with post-order bidirectional iterator
    // and O(h) post-order random access.
    // Alloc should be of type basic_polish_node.
    template<typename BasicNode, typename Alloc = std::allocator<BasicNode>>
    class slicing_tree {
        using self = slicing_tree;

    protected:
        using traits = impl::node_traits<BasicNode>;
        using node_type = typename traits::node_type;

    public:
        using combine_type = typename traits::combine_type;
        using dimension_type = typename traits::dimension_type;
        using value_type = typename traits::value_type;
        using size_type = typename traits::size_type;

        using allocator_type = Alloc;
        using iterator = slicing_tree_const_iterator<traits>;
        using const_iterator = slicing_tree_const_iterator<traits>;

        slicing_tree() : slicing_tree(allocator_type()) {}

        explicit slicing_tree(const allocator_type &alloc) :
            alloc_(alloc) {
            header_ = copy_node(node_type::make_header());
        }

        slicing_tree(const self &other) :
            alloc_(std::allocator_traits<allocator_type>::
                select_on_container_copy_construction(other.alloc_)) {
            header_ = copy_tree(other.header_);
        }

        ~slicing_tree() {
            clear_tree(header_);
        }

        self &operator=(const self &other) {
            if (std::allocator_traits<allocator_type>::
                propagate_on_container_copy_assignment::value
                && alloc_ != other.alloc_) {
                clear_tree(header_);
                alloc_ = other.alloc_;
                header_ = copy_tree(other.header_);
            } else {
                // NOTE: self-assignment goes here
                node_type *new_root = copy_tree(other.header_->lc());
                clear();
                attach_left(header_, new_root);
            }
            return *this;
        }

        // Construct tree with a list of modules and a polish expression.
        // e.g., size(modules) == 3, expr == {0, 1, *, 1, +, 2, *}.
        bool construct(const std::vector<yal::Module> &modules,
            const std::vector<expression::polish_expression_type> &expr) {
            return construct(modules.begin(), expr.begin(), expr.end());
        }

        // Construct tree with a list of modules and a polish expression.
        template<typename RanIt, typename InIt>
        bool construct(RanIt first_module, InIt first_expr, InIt last_expr) {
            std::vector<node_type *> stack;
            if (std::is_convertible<
                typename std::iterator_traits<InIt>::iterator_category,
                std::forward_iterator_tag>::value)
                stack.reserve(std::distance(first_expr, last_expr));
            bool pass = true;

            for (; first_expr != last_expr; ++first_expr) {
                expression::polish_expression_type e = *first_expr;
                if (e != expression::COMBINE_HORIZONTAL
                    && e != expression::COMBINE_VERTICAL) {
                    const yal::Module &m = first_module[e];
                    stack.push_back(new_leaf(m));
                } else {
                    if (stack.size() < 2) {
                        pass = false;
                        break;
                    }
                    node_type *t2 = stack.back();
                    stack.pop_back();
                    node_type *t1 = stack.back();
                    stack.pop_back();
                    combine_type combine = e == expression::COMBINE_HORIZONTAL ?
                        combine_type::HORIZONTAL : combine_type::VERTICAL;
                    node_type *t = new_operator(combine);
                    attach_left(t, t1);
                    attach_right(t, t2);
                    t->count_size();
                    t->count_area();
                    stack.push_back(t);
                }
            }

            pass = pass && stack.size() == 1;
            if (pass) {
                clear();
                attach_left(header_, stack.back());
            } else {
                for (node_type *p : stack)
                    clear_tree(p);
            }
            return pass;
        }

        void clear() {
            clear_tree(header_->lc());
            header_->lc() = nullptr;
        }

        bool empty() const noexcept {
            return !header_->lc();
        }

        size_type size() const noexcept {
            return empty() ? 0 : header_->lc()->size;
        }

        // Get kth iterator with post-order index.
        const_iterator get(size_type k) const {
            return k >= size() ? end() : const_iterator(get_impl(header_->lc(), k));
        }

        // STL-like begin.
        const_iterator begin() const noexcept {
            const node_type *t = header_;
            while (t->lc())
                t = t->lc();
            return const_iterator(t);
        }

        // STL-like end.
        const_iterator end() const noexcept {
            return const_iterator(header_);
        }

        // Conduct M1 / M3 change.
        // NOTE: if we want to do leaf-operator swap,
        // then pos1 == std::prev(pos2) (in O(1)).
        // @require pos1 <= pos2; if M3, pos1 == std::prev(pos2)
        // @return true iff operation valid
        bool swap_nodes(const_iterator pos1, const_iterator pos2) {
            if (pos1 == pos2 || pos1 == end() || pos2 == end())
                return false;

            bool ret = true;
            node_type *t1 = get_iter_pointer(pos1), *t2 = get_iter_pointer(pos2);
            switch ((is_leaf(t1) << 1) | is_leaf(t2)) {
            case 0:
                // Neither is leaf, error.
                ret = false;
                break;
            case 1:
                // t2 is leaf, conduct M3 change.
                ret = swap_operator_leaf(t1, t2);
                break;
            case 2:
                // t1 is leaf, try conducting M3 change.
                ret = swap_leaf_operator(t1, t2);
                break;
            case 3:
                // Both are leaves, conduct M1 change.
                swap_leaves(t1, t2);
                break;
            default:
                assert(false);
            }
            return ret;
        }

        // Conduct M2 change.
        // @return false iff pos points to header_ or the node is not an operator
        //         (i.e., is a leaf)
        bool invert_chain(const_iterator pos) {
            node_type *t = get_iter_pointer(pos);
            if (t == header_ || is_leaf(t))
                return false;
            while (t != header_) {
                t->invert_combine_type();
                t->count_area();
                t = t->parent();
            }
            return true;
        }

        // For debug.
        std::ostream &print_tree(std::ostream &os, int ident = 4,
            char fill = ' ') const {
            if (!empty())
                header_->lc()->print_tree(os, 0, ident, fill);
            return os;
        }

        // For debug.
        bool check_integrity() const {
            return empty() || check_integrity_impl(header_->lc());
        }

    protected:
        template<typename... Types>
        node_type *new_node(Types &&...args) {
            node_type *t = alloc_.allocate(1);
            ::new (t) node_type(std::forward<Types>(args)...);
            return t;
        }

        node_type *new_leaf(const yal::Module &m) {
            node_type *t = alloc_.allocate(1);
            traits::placement_new_leaf(t, m, alloc_);
            return t;
        }

        node_type *new_operator(combine_type type) {
            node_type *t = alloc_.allocate(1);
            traits::placement_new_operator(t, type, alloc_);
            return t;
        }

        node_type *copy_node(const node_type &src) {
            return new_node(src);
        }

        node_type *copy_node(const node_type *src) {
            return copy_node(*src);
        }

        void delete_node(node_type *t) {
            t->~node_type();
            alloc_.deallocate(t, 1);
        }

        static void attach_left(node_type *father, node_type *left) {
            father->lc() = left;
            left->parent() = father;
        }

        static void attach_right(node_type *father, node_type *right) {
            father->rc() = right;
            right->parent() = father;
        }

        node_type *copy_tree(const node_type *src) {
            if (!src)
                return nullptr;
            node_type *dst = copy_node(src);
            if (src->lc())
                attach_left(dst, copy_tree(src->lc()));
            if (src->rc())
                attach_right(dst, copy_tree(src->rc()));
            return dst;
        }

        void clear_tree(node_type *t) {
            if (t) {
                clear_tree(t->lc());
                clear_tree(t->rc());
                delete_node(t);
            }
        }

        static bool is_leaf(const node_type *t) noexcept {
            return t->is_leaf();
        }

        static void count_size(node_type *t, std::true_type) noexcept {
            t->count_size();
        }

        static void count_size(node_type *t, std::false_type) noexcept {}

        static node_type *get_iter_pointer(const_iterator pos) noexcept {
            return const_cast<node_type *>(pos.ptr_);
        }

        // Get node with post-order index.
        const node_type *get_impl(const node_type *t, size_type offset) const {
            assert(t && offset < t->size);
            for (;;) {
                size_type lc_sz = t->lc() ? t->lc()->size : 0;
                if (offset < lc_sz) {
                    t = t->lc();
                } else if (offset < t->size - 1) {
                    t = t->rc();
                    offset -= lc_sz;
                } else {
                    return t;
                }
            }
        }

        // Update down-top from non-leaf node t.
        template<bool B>
        void update_downtop(node_type *t,
            std::integral_constant<bool, B> update_size) {
            assert(!is_leaf(t));
            while (t != header_) {
                t->count_area();
                count_size(t, update_size);
                t = t->parent();
            }
        }

        // Update down-top simultaneously from non-leaf nodes t1 and t2.
        // Some nodes along the path may be updated twice.
        template<bool B>
        void update_downtop(node_type *t1, node_type *t2,
            std::integral_constant<bool, B> update_size) {
            assert(!is_leaf(t1) && !is_leaf(t2));
            while (t1 != header_ && t2 != header_) {
                t1->count_area();
                count_size(t1, update_size);
                t2->count_area();
                count_size(t2, update_size);
                t1 = t1->parent();
                t2 = t2->parent();
            }
            update_downtop(t1, update_size);
            update_downtop(t2, update_size);
        }

        void swap_leaves(node_type *t1, node_type *t2) {
            node_type *p1 = t1->parent(), *p2 = t2->parent();
            (t2 == p2->lc() ? p2->lc() : p2->rc()) = t1;
            (t1 == p1->lc() ? p1->lc() : p1->rc()) = t2;
            t1->parent() = p2;
            t2->parent() = p1;
            update_downtop(p1, p2, std::false_type());
        }

        // Conduct M3 change, given post-order index(t2) - index(t1) == 1.
        // case a:
        //           |
        //           p1
        //         /    \
        //      t1        *
        //     /  \      / \
        //   ...  ...  ...  ...
        //             / \
        //            p2 ...
        //           / \
        //          t2  ...
        // case b:
        //           |
        //           p1
        //         /    \
        //      t1        t2
        //     /  \
        //   ...  ...
        // @param t1 operator
        // @param t2 leaf
        // @return true (always)
        bool swap_operator_leaf(node_type *t1, node_type *t2) {
            assert(!is_leaf(t1) && is_leaf(t2));
            assert(t1 == t1->parent()->lc());
            assert(t2 == t2->parent()->lc() || t1->parent() == t2->parent());
            node_type *p1 = t1->parent(), *p2 = t2->parent();
            attach_left(p1, t1->lc());
            t1->lc() = t1->rc();
            attach_right(t1, t2);
            if (p1 != p2)
                attach_left(p2, t1);    // case a
            else
                attach_right(p2, t1);   // case b
            update_downtop(t1, std::true_type()); // one-way update should suffice
            return true;
        }

        // Conduct M3 change, given post-order index(t2) - index(t1) == 1.
        // case a:
        //            |
        //           ca
        //         /     \
        //      pre       *
        //     /  \      /  \
        //   ...  ...  ...  ...
        //             /
        //            t2
        //           /  \
        //         ...   t1
        // case b:
        //            |
        //           ca
        //         /     \
        //      pre       t2
        //     /  \      /  \
        //   ...  ...  ...   t1
        // @param t1 leaf
        // @param t2 operator
        // @return true iff operation valid
        bool swap_leaf_operator(node_type *t1, node_type *t2) {
            if (t1 != t2->rc())
                return false;

            node_type *pre = t2, *ca = t2->parent();
            while (pre == ca->lc()) {
                pre = ca;
                ca = ca->parent();
                if (!ca)
                    return false;
            }
            pre = ca->lc();
            if (t2->parent() != ca)
                attach_left(t2->parent(), t1);    // case a
            else
                attach_right(t2->parent(), t1);   // case b
            t2->rc() = t2->lc();
            attach_left(t2, pre);
            attach_left(ca, t2);
            // NOTE: sometimes one-way update suffices
            update_downtop(t2, t1->parent(), std::true_type());
            return true;
        }

        bool check_integrity_impl(const node_type *t) const {
            if (t->is_leaf())
                return t->check_area() && t->check_size();
            return check_integrity_impl(t->lc()) && check_integrity_impl(t->rc())
                && t->check_area() && t->check_size()
                && t->lc()->parent() == t && t->rc()->parent() == t;
        }

        // ʹ��allocator����new / delete, ���㳣���Ż� (SA��������Ҫ���Ƶ����Ž�)
        // NOTE: 1. δ��empty-base optimization; 2. ����header_ǰ�汣��һ�� (ctor)
        typename std::allocator_traits<Alloc>::
            template rebind_alloc<node_type> alloc_;

        // header_->left == root, ���˳�header_�����нڵ㶼�и��ڵ�, Ҳ���ܼ���һЩ�ж�
        node_type *header_;
    };

    // A polish_tree is a tree of basic_polish_node
    // (internally tree_node<basic_polish_node>).
    template<typename Alloc = std::allocator<basic_polish_node>>
    class polish_tree : public slicing_tree<basic_polish_node, Alloc> {
        using self = polish_tree;
        using base = slicing_tree<basic_polish_node, Alloc>;

    protected:
        using typename base::traits;
        using typename base::node_type;

    public:
        using typename base::combine_type;
        using typename base::dimension_type;
        using typename base::value_type;
        using typename base::size_type;
        using typename base::allocator_type;
        using typename base::iterator;
        using typename base::const_iterator;

        using slicing_tree<basic_polish_node, Alloc>::slicing_tree;

        // Rotate leaf and update down-top.
        bool rotate_leaf(const_iterator pos) noexcept {
            node_type *t = this->get_iter_pointer(pos);
            if (t == this->header_ || !base::is_leaf(t))
                return false;
            t->invert_combine_type();
            while (t != this->header_) {
                t->count_area();
                t = t->parent();
            }
            return true;
        }

    };

    // A vectorized_polish_tree is a tree of basic_vectorized_polish_node<CoordAlloc>
    // (internally tree_node<basic_vectorized_polish_node<CoordAlloc>>).
    template<typename CoordAlloc = std::allocator<meta_polish_node::coord_type>,
        typename NodeAlloc = std::allocator<basic_vectorized_polish_node<CoordAlloc>>>
    using vectorized_polish_tree = slicing_tree<
        basic_vectorized_polish_node<CoordAlloc>, NodeAlloc>;
}

#endif /* polish_tree_hpp */
