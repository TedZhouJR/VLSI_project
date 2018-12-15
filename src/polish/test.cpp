// test.cpp: testcases for slicing tree.
// Author: LYL

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#include "polish_tree.hpp"

using namespace std;
using namespace polish;

#define MY_ASSERT(expr) !!(expr) || error(#expr, __LINE__)
#define SHOW_STARTING_TEST() do { \
cout << "Starting " << __FUNCTION__ << "..." << endl; } while (false)

namespace {

    bool error(const char *expr, int line) {
        cerr << "Debug assertion failed: " << expr 
            << ", test.cpp (" << line << ")" << endl;
        exit(EXIT_FAILURE);
        return false;
    }

    class TestCase {
    public:
        using combine_type = typename basic_polish_node::combine_type;
        using tree_type = polish::polish_tree<>;
        using vctr_tree_type = polish::vectorized_polish_tree<>;

        TestCase() : eng_(std::random_device{}()), line_(80, '-') {}

        void test_tree_basic() {
            SHOW_STARTING_TEST();
            test_basic(tree_);
        }

        void test_vtree_basic() {
            SHOW_STARTING_TEST();
            test_basic(vtree_);
        }

        void test_tree_m3_0(std::size_t sz = 33, std::size_t times = 16) {
            SHOW_STARTING_TEST();
            test_m3_0(tree_, sz, times);
        }

        void test_vtree_m3_0(std::size_t sz = 33, std::size_t times = 16) {
            SHOW_STARTING_TEST();
            test_m3_0(vtree_, sz, times);
        }

        void test_tree_m3_1(std::size_t sz = 33, std::size_t times = 16) {
            SHOW_STARTING_TEST();
            test_m3_1(tree_, sz, times);
        }

        void test_vtree_m3_1(std::size_t sz = 33, std::size_t times = 16) {
            SHOW_STARTING_TEST();
            test_m3_1(tree_, sz, times);
        }

        void test_curve() {
            SHOW_STARTING_TEST();
            std::allocator<typename polish::meta_polish_node::coord_type> alloc;
            polish::basic_vectorized_polish_node<> p(combine_type::VERTICAL, alloc),
                lc(combine_type::LEAF, alloc), rc(combine_type::LEAF, alloc);
            lc.points = {{1, 5}, {3, 2}, {5, 0}};
            rc.points = {{2, 3}, {4, 1}, {5, 0}};

            cout << "Curve f:" << endl;
            print_coord_list(lc.points) << endl;
            cout << "Curve g:" << endl;
            print_coord_list(rc.points) << endl;

            for (auto type : { combine_type::VERTICAL, combine_type::HORIZONTAL }) {
                p.type = type;
                p.count_area(lc, rc);
                cout << "f " << (type == combine_type::VERTICAL ? "+" : "*") 
                    << " g:" << endl;
                print_coord_list(p.points) << endl;
            }

            cout << line_ << endl;
        }

        void test_tree_rotate_leaf() {
            SHOW_STARTING_TEST();
            load_basic();
            tree_.construct(modules_, expr_);

            auto root = std::prev(tree_.end());
            auto w = root->width, h = root->height;
            for (auto i = tree_.begin(); i != tree_.end(); ++i) {
                if (i->type != combine_type::LEAF) {
                    MY_ASSERT(!tree_.rotate_leaf(i));
                    continue;
                }
                MY_ASSERT(tree_.rotate_leaf(i));
                MY_ASSERT(tree_.rotate_leaf(i));
                MY_ASSERT(root == std::prev(tree_.end()));
                MY_ASSERT(w == root->width && h == root->height);
            }
            MY_ASSERT(!tree_.rotate_leaf(tree_.end()));
            cout << line_ << endl;
        }

    private:
        template<typename Tree>
        static auto get(Tree &&t, std::size_t k) {
            return std::next(t.begin(), k);
        }

        template<typename Tree>
        static std::ostream &print_tree(const Tree &t, 
            std::ostream &os = std::cout) {
            for (const auto &e : t)
                os << e << " ";
            os << endl;
            return t.print_tree(os, 2);
        }

        template<typename Tree>
        static bool is_m3_valid(const Tree &t, uint32_t i) {
            if (i + 1 >= std::distance(t.begin(), t.end()))
                return false;
            auto p = get(t, i), q = std::next(p);
            MY_ASSERT(p == std::prev(q));
            if ((p->type == combine_type::LEAF) == (q->type == combine_type::LEAF))
                return false;
            if (q->type == combine_type::LEAF)  // operator-leaf swap
                return true;
            // leaf-operator swap
            auto prefix_oprerators = 1 + std::count_if(t.begin(), q,
                [](const auto &e) { return e.type != combine_type::LEAF; });
            return 2 * prefix_oprerators < i + 1;
        }

        template<typename Cont>
        static std::ostream &print_coord_list(Cont &&cont, 
            std::ostream &os = std::cout) {
            for (auto &&e : cont)
                cout << "(" << e.first << "," << e.second << ") ";
            return os;
        }

        bool test_traversal(const tree_type &t) {
            buf_.assign(t.begin(), t.end());
            return std::equal(buf_.rbegin(), buf_.rend(),
                std::make_reverse_iterator(t.end()), 
                [](const auto &x, const auto &y) {
                return x.type == y.type && x.width == y.width 
                    && x.height == y.height;
            });
        }

        bool test_traversal(const vctr_tree_type &t) {
            vbuf_.assign(t.begin(), t.end());
            return std::equal(vbuf_.rbegin(), vbuf_.rend(),
                std::make_reverse_iterator(t.end()),
                [](const auto &x, const auto &y) {
                return x.type == y.type && x.points == y.points;
            });
        }

        void load_basic() {
            yal::Module m;
            m.xpos.push_back(0);
            m.ypos.push_back(0);
            m.xpos.push_back(30);
            m.ypos.push_back(20);
            modules_.assign(6, m);
            expr_ = {
                0, 1, expression::COMBINE_HORIZONTAL,
                2, 3, expression::COMBINE_VERTICAL, 4, 5,
                expression::COMBINE_VERTICAL, expression::COMBINE_HORIZONTAL,
                expression::COMBINE_VERTICAL
            };
        }

        template<typename Tree>
        void test_basic(Tree &t) {
            load_basic();
            t.construct(modules_, expr_);
            MY_ASSERT(test_traversal(t));
            MY_ASSERT(t.check_integrity());
            cout << "Original:" << endl;
            print_tree(t) << endl << line_ << endl;

            {
                auto tmp = t;
                tmp.swap_nodes(get(tmp, 4), get(tmp, 6));
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M1:" << endl;
                print_tree(tmp) << endl << line_ << endl;
            }

            {
                auto tmp = t;
                tmp.invert_chain(get(tmp, 8));
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M2:" << endl;
                print_tree(tmp) << endl << line_ << endl;
            }

            {
                auto tmp = t;
                auto i = get(tmp, 2), j = std::next(i);
                tmp.swap_nodes(i, j);
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M3:" << endl;
                print_tree(tmp) << endl << line_ << endl;

                cout << "M3 again:" << endl;
                tmp.swap_nodes(j, i);
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                print_tree(tmp) << endl << line_ << endl;
            }
        }

        template<typename Tree>
        void test_m3_0(Tree &t, std::size_t sz, std::size_t times) {
            // Construct a left-leaning tree.
            modules_.resize(1);
            if (!(sz & 1))
                ++sz;
            if (sz < 3)
                sz = 3;
            expr_.assign(sz, 0);
            {
                int state = -1;
                for (auto &e : expr_) {
                    if (state == 1) {
                        e = expression::COMBINE_VERTICAL;
                    } else if (state == 3) {
                        e = expression::COMBINE_HORIZONTAL;
                    }
                    if (++state == 4)
                        state = 0;
                }
            }

            test_m3_impl(t, times);
        }

        template<typename Tree>
        void test_m3_1(Tree &t, std::size_t sz, std::size_t times) {
            // Construct a right-leaning tree.
            modules_.resize(1);
            if (!(sz & 1))
                ++sz;
            if (sz < 3)
                sz = 3;
            expr_.assign(sz, 0);
            for (auto i = sz / 2 + 1; i != sz; ++i) {
                expr_[i] = (i & 1) ? expression::COMBINE_HORIZONTAL
                    : expression::COMBINE_VERTICAL;
            }

            test_m3_impl(t, times);
        }

        template<typename Tree>
        void test_m3_impl(Tree &t, std::size_t times) {
            t.construct(modules_, expr_);
            MY_ASSERT(test_traversal(t));
            MY_ASSERT(t.check_integrity());
            print_tree(t) << endl << line_ << endl;

            uniform_int_distribution<typename tree_type::size_type> 
                rand(0, std::distance(t.begin(), t.end()) - 2);

            size_t threshold = 1 << 16;
            while (threshold-- && times) {
                auto idx = rand(eng_);
                auto i = get(t, idx), j = get(t, idx + 1);
                MY_ASSERT(i == std::prev(j));
                MY_ASSERT(j == std::next(i));
                if (i->type == combine_type::LEAF 
                    ^ j->type == combine_type::LEAF) {
                    cout << "Swap [" << idx << "] " << *i 
                        << " and " << "[" << (idx + 1) << "] " << *j << " ";
                    bool valid = is_m3_valid(t, idx);
                    if (!t.swap_nodes(i, j)) {
                        cout << "failed." << endl;
                        MY_ASSERT(!valid);
                    } else {
                        cout << "succeeded:" << endl;
                        print_tree(t) << endl;
                        MY_ASSERT(valid);
                        MY_ASSERT(test_traversal(t));
                        MY_ASSERT(t.check_integrity());
                        auto t2 = t;
                        auto ret = t2.swap_nodes(get(t2, idx), get(t2, idx + 1));
                        MY_ASSERT(ret);
                        MY_ASSERT(test_traversal(t2));
                        MY_ASSERT(t2.check_integrity());
                        --times;
                    }
                    cout << line_ << endl;
                }
            }
        }

        std::vector<yal::Module> modules_;
        std::vector<expression::polish_expression_type> expr_;
        std::vector<basic_polish_node> buf_;
        std::vector<basic_vectorized_polish_node<>> vbuf_;
        tree_type tree_;
        vctr_tree_type vtree_;
        default_random_engine eng_;
        std::string line_;
    };

}

int main(int argc, char **argv) {
    cout << "Start testing..." << endl;
    TestCase testcase;
    testcase.test_curve();
    testcase.test_tree_basic();
    testcase.test_vtree_basic();
    testcase.test_tree_m3_0();
    testcase.test_vtree_m3_0();
    testcase.test_tree_m3_1();
    testcase.test_vtree_m3_1();
    testcase.test_tree_rotate_leaf();
    cout << "All tests passed!" << endl;
    return 0;
}