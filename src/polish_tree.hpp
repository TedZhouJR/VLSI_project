//  polish_tree.hpp
//  sa

#ifndef polish_tree_hpp
#define polish_tree_hpp

#include <cassert>
#include <cstddef>
#include <vector>

#include "polish_node.hpp"
#include "yal/module.h"

namespace polish {

    namespace expression {
        // Polish expression is a sequence of integers, each representing
        // either an operator (HORIZONTAL_COMBINE or VERTICAL_COMBINE) 
        // or a non-negative module index.
        using polish_expression_type = int;
        constexpr polish_expression_type HORIZONTAL_COMBINE = -1;
        constexpr polish_expression_type VERTICAL_COMBINE = -2;
    }

    template<typename Alloc = std::allocator<polish_node>>
    class polish_tree {
    public:
        using allocator_type = typename std::allocator_traits<Alloc>::
            template rebind_alloc<polish_node>;
        using node_type = polish_node;
        using size_type = typename node_type::size_type;

        polish_tree() : polish_tree(allocator_type()) {}

        explicit polish_tree(const allocator_type &alloc) :
            alloc_(alloc) {
            header_ = new_node(node_type::NO_COMBINE, 0, 0);
        }

        polish_tree(const polish_tree &other) :
            alloc_(std::allocator_traits<allocator_type>::
                select_on_container_copy_construction(other.alloc_)),
            header_(copy_tree(other.header_)) {
        }

        ~polish_tree() {
            clear_impl(header_);
        }

        polish_tree &operator=(const polish_tree &other) {
            if (std::allocator_traits<allocator_type>::
                propagate_on_container_copy_assignment::value
                && alloc_ != other.alloc_) {
                clear_impl(header_);
                alloc_ = other.alloc_;
            }
            header_ = copy_tree(other.header_);
            return *this;
        }

        // Construct tree with a list of modules and a polish expression.
        // e.g., size(modules) == 3, expr == {0, 1, *, 1, +, 2, *}.
        // Can be templated if necessary.
        void construct(const std::vector<yal::Module> &modules, 
            const std::vector<expression::polish_expression_type> &expr) {
            clear();
            std::vector<node_type *> stack;
            stack.reserve(expr.size());

            for (expression::polish_expression_type e : expr) {
                if (e != expression::HORIZONTAL_COMBINE && e != expression::VERTICAL_COMBINE) {
                    const yal::Module &m = modules[e];
                    stack.push_back(new_node(node_type::NO_COMBINE, m.xspan(), m.yspan()));
                } else {
                    node_type *t2 = stack.back();
                    stack.pop_back();
                    node_type *t1 = stack.back();
                    stack.pop_back();
                    char combine = e == expression::HORIZONTAL_COMBINE ?
                        node_type::HORIZONTAL_COMBINE : node_type::VERTICAL_COMBINE;
                    node_type *t = new_node(combine, 0, 0);
                    attach_left(t, t1);
                    attach_right(t, t2);
                    t->size = t1->size + t2->size + 1;
                    t->count_area();
                    stack.push_back(t);
                }
            }

            assert(stack.size() == 1);
            attach_left(header_, stack.back());
        }

        void clear() {
            clear_impl(header_->lc);
            header_->lc = nullptr;
        }

        bool empty() const noexcept {
            return !header_->lc;
        }

        size_type size() const noexcept {
            return empty() ? 0 : header_->lc->size;
        }

        const node_type *get(size_type k) const {
            return k >= size() ? nullptr : get_impl(header_->lc, k);
        }

        node_type *get(size_type k) {
            return const_cast<node_type *>(
                const_cast<const polish_tree *>(this)->get(k));
        }

    private:
        node_type *new_node(char combine_type, double width, double height) {
            node_type *t = alloc_.allocate(1);
            ::new (t) node_type(combine_type, height, width);
            return t;
        }

        node_type *copy_node(const node_type *src) {
            node_type *t = alloc_.allocate(1);
            ::new (t) node_type(*src);
            return t;
        }

        void delete_node(node_type *t) {
            t->~node_type();
            alloc_.deallocate(t, 1);
        }

        static void attach_left(node_type *parent, node_type *left) {
            parent->lc = left;
            left->parent = parent;
        }

        static void attach_right(node_type *parent, node_type *right) {
            parent->rc = right;
            right->parent = parent;
        }

        node_type *copy_tree(const node_type *src) {
            if (!src)
                return nullptr;
            node_type *dst = copy_node(src);
            if (src->lc)
                attach_left(dst, copy_tree(src->lc));
            if (src->rc)
                attach_right(dst, copy_tree(src->rc));
            return dst;
        }

        void clear_impl(polish_node *t) {
            if (t) {
                clear_impl(t->lc);
                clear_impl(t->rc);
                delete_node(t);
            }
        }

        const node_type *get_impl(const node_type *t, size_type offset) const {
            assert(t && offset < t->size);
            for (;;) {
                size_type lc_sz = t->lc ? t->lc->size : 0;
                if (offset < lc_sz) {
                    t = t->lc;
                } else if (offset > lc_sz) {
                    t = t->rc;
                    offset -= (lc_sz + 1);
                } else {
                    return t;
                }
            }
        }

        // header_->left == root, 如此除header_外所有节点都有父节点, 也许能减少一些判断
        node_type *header_;

        // 使用allocator管理new / delete, 方便常数优化 (SA过程中需要复制到最优解)
        // NOTE: 未作empty-base optimization
        allocator_type alloc_;
    };

}

#endif /* polish_tree_hpp */
