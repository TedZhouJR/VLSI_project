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

        TestCase() : eng_(std::random_device{}()), line_(80, '-') {}

        void test_basic() {
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
            tree_.construct(modules_, expr_);
            MY_ASSERT(test_traversal(tree_));
            MY_ASSERT(tree_.check_integrity());
            cout << "Original:" << endl;
            print_tree(tree_) << endl << line_ << endl;

            {
                auto tmp = tree_;
                tmp.swap_nodes(tmp.get(4), tmp.get(6));
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M1:" << endl;
                print_tree(tmp) << endl << line_ << endl;
            }

            {
                auto tmp = tree_;
                tmp.invert_chain(tmp.get(8));
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M2:" << endl;
                print_tree(tmp) << endl << line_ << endl;
            }

            {
                auto tmp = tree_;
                auto i = tmp.get(2), j = std::next(i);
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

        void test_m3_0(typename tree_type::size_type sz = 33, 
            std::size_t times = 16) {
            // Construct a left-leaning tree.
            modules_.resize(1);
            if (!(sz & 1))
                ++sz;
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
            
            test_m3_impl(times);
        }

        void test_m3_1(typename tree_type::size_type sz = 33, 
            std::size_t times = 16) {
            // Construct a right-leaning tree.
            modules_.resize(1);
            if (!(sz & 1))
                ++sz;
            expr_.assign(sz, 0);
            for (auto i = sz / 2 + 1; i != sz; ++i) {
                expr_[i] = (i & 1) ? expression::COMBINE_HORIZONTAL
                    : expression::COMBINE_VERTICAL;
            }
            
            test_m3_impl(times);
        }

    private:
        static std::ostream &print_tree(const tree_type &t, 
            std::ostream &os = std::cout) {
            for (const basic_polish_node &e : t)
                os << e << " ";
            os << endl;
            return t.print_tree(os, 2);
        }

        // Use dirty cast to get the underlying polish_node.
        static const polish_node *iter_to_polish_node(
            polish_tree_const_iterator iter) {
            return static_cast<const polish_node *>(&(*iter));
        }

        static bool is_m3_valid(const tree_type &t, uint32_t i) {
            if (i + 1 >= t.size())
                return false;
            auto p = t.get(i), q = std::next(p);
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

        bool test_traversal(const tree_type &t) {
            buf_.assign(t.begin(), t.end());
            return std::equal(buf_.rbegin(), buf_.rend(),
                std::make_reverse_iterator(t.end()), 
                [](const auto &x, const auto &y) {
                return x.type == y.type && x.width == y.width 
                    && x.height == y.height;
            });
        }

        bool test_m3_impl(std::size_t times) {
            tree_.construct(modules_, expr_);
            MY_ASSERT(test_traversal(tree_));
            MY_ASSERT(tree_.check_integrity());
            print_tree(tree_) << endl << line_ << endl;

            uniform_int_distribution<typename tree_type::size_type> 
                rand(0, tree_.size() - 2);

            size_t threshold = 1 << 16;
            while (threshold-- && times) {
                auto idx = rand(eng_);
                auto i = tree_.get(idx), j = tree_.get(idx + 1);
                MY_ASSERT(i == std::prev(j));
                MY_ASSERT(j == std::next(i));
                if (i->type == combine_type::LEAF 
                    ^ j->type == combine_type::LEAF) {
                    cout << "Swap [" << idx << "] " << *i 
                        << " and " << "[" << (idx + 1) << "] " << *j << " ";
                    bool valid = is_m3_valid(tree_, idx);
                    if (!tree_.swap_nodes(i, j)) {
                        cout << "failed." << endl;
                        MY_ASSERT(!valid);
                    } else {
                        cout << "succeeded:" << endl;
                        print_tree(tree_) << endl;
                        MY_ASSERT(valid);
                        MY_ASSERT(test_traversal(tree_));
                        MY_ASSERT(tree_.check_integrity());
                        auto t2 = tree_;
                        auto ret = t2.swap_nodes(t2.get(idx), t2.get(idx + 1));
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
        polish::polish_tree<> tree_;
        default_random_engine eng_;
        std::string line_;
    };

}

int main(int argc, char **argv) {
    cout << "Hello" << endl;
    TestCase testcase;
    testcase.test_basic();
    testcase.test_m3_0();
    testcase.test_m3_1();
    return 0;
}