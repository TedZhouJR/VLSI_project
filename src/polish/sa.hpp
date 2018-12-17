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
        static std::ostream &print_tree(const Tree &t,
            std::ostream &os = std::cout) {
            for (const auto &e : t)
                os << e << " ";
            os << endl;
            return t.print_tree(os, 2);
        }

        template<typename Tree>
        static bool is_m3_valid(const Tree &t, uint32_t i) {
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
                tmp.swap_nodes(tmp.get(4), tmp.get(6));
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M1:" << endl;
                print_tree(tmp) << endl << line_ << endl;
            }

            {
                auto tmp = t;
                tmp.invert_chain(tmp.get(8));
                MY_ASSERT(test_traversal(tmp));
                MY_ASSERT(tmp.check_integrity());
                cout << "M2:" << endl;
                print_tree(tmp) << endl << line_ << endl;
            }

            {
                auto tmp = t;
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
                rand(0, t.size() - 2);

            size_t threshold = 1 << 16;
            while (threshold-- && times) {
                auto idx = rand(eng_);
                auto i = t.get(idx), j = t.get(idx + 1);
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
        std::vector<basic_vectorized_polish_node<>> vbuf_;
        tree_type tree_;
        vctr_tree_type vtree_;
        default_random_engine eng_;
        std::string line_;
    };

    enum class operation_type {
        M1, M2, M3
    };

    struct operation {
        operation_type type;
        int target1;
        int target2;
    };

    class SA {
    public:
        using combine_type = typename basic_polish_node::combine_type;
        using tree_type = polish::polish_tree<>;
        using vctr_tree_type = polish::vectorized_polish_tree<>;
        using const_iterator = typename vctr_tree_type::const_iterator;

        SA(vctr_tree_type* vtree_in, float init_accept_rate, float cooldown_speed_in,
            int balance_minstep_in, float ending_temperature_in)
        : /*eng_(std::random_device{}()), */line_(80, '-') {
            vtree_ = *vtree_in;
            srand((unsigned)time(NULL));
            init_vbuf();
            print();
            print_tree(vtree_);
            temperature = count_init_temprature(init_accept_rate);
            std::cout << "init temperature " << temperature << endl;
            cooldown_speed = cooldown_speed_in;
            accept_under_currentT = total_under_currentT = 0;
            balance_minstep = balance_minstep_in;
            ending_temperature = ending_temperature_in;
            best_solution = -1;
        }

        void take_step() {
            // std::cout << "step" << endl;
            float pre_min_area, post_min_area;
            pre_min_area = count_min_area();
            struct operation op = random_operation();
            struct operation op_final = check_valid_and_go(op);
            post_min_area = count_min_area();
            if (pre_min_area <= post_min_area) { //probably accept
                float acc_rate = exp((pre_min_area - post_min_area) / temperature);
                if (((float)rand() / (float)RAND_MAX) <= acc_rate) {
                    accept_under_currentT++;
                    total_under_currentT++;
                } else {
                    goto_neighbor(op_final);   //recover previous state
                    total_under_currentT++;
                }
            } else { //accept
                accept_under_currentT++;
                total_under_currentT++;
            }
        }

        //only if accept rate > constant1 and total step > constant2
        bool reach_balance() {
            return accept_under_currentT > balance_minstep;
        }

        void cool_down_by_ratio() {
            std::cout << "current area : " << count_min_area() << endl;
            temperature = temperature * (1 - cooldown_speed);
            accept_under_currentT = total_under_currentT = 0;
        }

        void cool_down_by_speed() {
            std::cout << "current area : " <<  count_min_area() << endl;
            temperature = temperature - cooldown_speed;
            accept_under_currentT = total_under_currentT = 0;
        }

        bool reach_end() {
            return temperature <= ending_temperature;
        }

        void print() {
            print_tree(vtree_);
        }

        float current_solution() {
            return best_solution;
        }

    private:
        template<typename Tree>
        static std::ostream &print_tree(const Tree &t,
            std::ostream &os = std::cout) {
            for (const auto &e : t)
                os << e << " ";
            os << endl;
            return t.print_tree(os, 2);
        }

        void init_vbuf() {
            const_iterator it = vtree_.begin();
            while (it != vtree_.end()) {
                vbuf_.push_back(it);
                it++;
            }
            std::cout << vbuf_.size() << endl;
            // vbuf_.assign(vtree_.begin(), vtree_.end());
        }

        void load_tree() {
            yal::Module m;
            m.xpos.push_back(0);
            m.ypos.push_back(0);
            m.xpos.push_back(30);
            m.ypos.push_back(20);
            modules_.assign(12, m);
            expr_ = {
                0, 1, expression::COMBINE_VERTICAL,
                2, 3, expression::COMBINE_VERTICAL, 4, 5,
                expression::COMBINE_VERTICAL, expression::COMBINE_VERTICAL,
                expression::COMBINE_VERTICAL,
                6, 7, expression::COMBINE_VERTICAL,
                8, 9, expression::COMBINE_VERTICAL, 10, 11,
                expression::COMBINE_VERTICAL, expression::COMBINE_VERTICAL,
                expression::COMBINE_VERTICAL, expression::COMBINE_VERTICAL
            };
            vtree_.construct(modules_, expr_);
        }

        float count_init_temprature(float init_accept_rate) {
            float init_min_area = count_min_area();
            float post_min_area, total_drop;
            int N = 100;
            for (int i = 0; i < N; i++) {
                struct operation op = random_operation();
                struct operation op_final = check_valid_and_go(op);
                print();
                post_min_area = count_min_area();
                total_drop += abs(init_min_area - post_min_area);
                goto_neighbor(op_final);   //recover previous state
            }
            return - total_drop / (100 * log(init_accept_rate));
        }

        float count_min_area() {
            // print_coord_list((std::prev(vbuf_in.end()))->points);
            float min_area = -1;
            for (auto &&e : vbuf_.back()->points) {
                if (e.first * e.second < min_area || min_area < 0) {
                    min_area = e.first * e.second;
                }
            }
            if (min_area < best_solution || best_solution < 0) {
                best_solution = min_area;
                best_tree = vtree_;
            }
            return min_area;
        }

        void goto_neighbor(struct operation op) {
            if (op.type == operation_type::M2) {
                vtree_.invert_chain(vtree_.get(op.target1));
            } else {
                vtree_.swap_nodes(vbuf_[op.target1], vbuf_[op.target2]);
                std::swap(vbuf_[op.target1], vbuf_[op.target2]);
            }
        }

        //检查操作合法性，校正并执行操作
        struct operation check_valid_and_go(struct operation op) {
            if (op.type == operation_type::M1) {
                // std::cout<< "step M1 " << op.target1 << endl;
                op.target1 = -1;
                while (op.target1 < 0) {
                    op.target2 = rand() % (vbuf_.size() - 1);
                    //如果是非叶节点，非法
                    if (vbuf_[op.target2]->type == combine_type::LEAF) {
                        //找左边的相邻叶节点，找不到则非法
                        for (op.target1 = op.target2 - 1; op.target1 >= 0; op.target1--) {
                            if (vbuf_[op.target1]->type == combine_type::LEAF)
                                break;
                        }
                    }
                }
                vtree_.swap_nodes(vbuf_[op.target1], vbuf_[op.target2]);
                std::swap(vbuf_[op.target1], vbuf_[op.target2]);
            } else if (op.type == operation_type::M2) {
                // std::cout<< "step M2 " << op.target1 << endl;
                //如果是叶节点，非法
                while (vbuf_[op.target1]->type == combine_type::LEAF) {
                    op.target1 = rand() % (vbuf_.size() - 1);
                }
                vtree_.invert_chain(vbuf_[op.target1]);
            } else if (op.type == operation_type::M3) {
                // std::cout<< "step M3 " << op.target1 << endl;
                bool valid = false;
                while (!valid) {
                    op.target2 = (rand() % (vbuf_.size() - 2)) + 1;
                    op.target1 = op.target2 - 1;
                    if ((vbuf_[op.target1]->type == combine_type::LEAF) xor
                        (vbuf_[op.target2]->type == combine_type::LEAF)) {
                        valid = vtree_.swap_nodes(vbuf_[op.target1], vbuf_[op.target2]);
                    }
                }
                std::swap(vbuf_[op.target1], vbuf_[op.target2]);
            }
            // std::cout<< "finished" << endl;
            return op;
        }

        struct operation random_operation() {
            struct operation op;
            int tmp = rand() % 3;
            if (tmp == 0) {
                op.type = operation_type::M1;
            } else if (tmp == 1) {
                op.type = operation_type::M2;
            } else {
                op.type = operation_type::M3;
            }
            tmp = rand() % (vbuf_.size() - 1);
            op.target1 = tmp;
            return op;
        }

        template<typename Cont>
        static std::ostream &print_coord_list(Cont &&cont,
            std::ostream &os = std::cout) {
            for (auto &&e : cont)
                cout << "(" << e.first << "," << e.second << ") ";
            return os;
        }

        std::vector<yal::Module> modules_;
        std::vector<expression::polish_expression_type> expr_;
        std::vector<const_iterator> vbuf_;
        vctr_tree_type vtree_, best_tree;
        default_random_engine eng_;
        std::string line_;
        float temperature, cooldown_speed;
        int accept_under_currentT, total_under_currentT;
        float ending_temperature;
        float best_solution;
        int balance_minstep;
    };

}
