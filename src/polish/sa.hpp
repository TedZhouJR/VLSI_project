// test.cpp: testcases for slicing tree.
// Author: LYL

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#include <boost/pool/pool_alloc.hpp>

#include "polish_tree.hpp"

using namespace std;
using namespace polish;

namespace {

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
        using vctr_tree_type = polish::vectorized_polish_tree<
            boost::fast_pool_allocator<polish::basic_vectorized_polish_node<
            boost::fast_pool_allocator<meta_polish_node::coord_type>>>>;
        using const_iterator = typename vctr_tree_type::const_iterator;

        SA(vctr_tree_type* vtree_in, int best_curve_in, double init_accept_rate, double cooldown_speed_in, double ending_temperature_in)
        {
            vtree_ = *vtree_in;
            srand((unsigned)time(NULL));
            srand(1);
            init_vbuf();
            temperature = count_init_temprature(init_accept_rate);
            std::cerr << "init temperature " << temperature << endl;
            cooldown_speed = cooldown_speed_in;
            accept_under_currentT = total_under_currentT = 0;
            ending_temperature = ending_temperature_in;
            best_solution = -1;
            tot_block_area = count_tot_block_area();
            best_curve = best_curve_in;
        }

        void take_step() {
            // std::cerr << "step" << endl;
            int pre_min_area, post_min_area;
            pre_min_area = count_min_area();
            struct operation op = random_operation();
            struct operation op_final = check_valid_and_go(op);
            post_min_area = count_min_area();
            if (pre_min_area <= post_min_area) { //probably accept
                double acc_rate = exp((pre_min_area - post_min_area) / temperature);
                if (((double)rand() / (double)RAND_MAX) <= acc_rate) {
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
            temperature = temperature * (1 - cooldown_speed);
            accept_under_currentT = total_under_currentT = 0;
        }

        void cool_down_by_speed() {
            temperature = temperature - cooldown_speed;
            accept_under_currentT = total_under_currentT = 0;
        }

        bool reach_end() {
            return temperature <= ending_temperature;
        }

        void print() {
            print_tree(vtree_);
        }

        double print_current_solution() {
            std::cerr << "minimum area is " << best_solution << endl;
            std::cerr << "utility is " << (double)tot_block_area / best_solution << endl;
            return (double)tot_block_area / best_solution;
        }

        vctr_tree_type show_best_tree () {
            return best_tree;
        }

        int show_best_curve() {
            return best_curve;
        }

    private:
        template<typename Tree>
        static std::ostream &print_tree(const Tree &t,
            std::ostream &os = std::cerr) {
            for (const auto &e : t)
                os << e << " ";
            os << endl;
            return t.print_tree(os, 2);
        }

        int count_tot_block_area() {
            int area = 0;
            for (auto it : vbuf_) {
                if (it->type == combine_type::LEAF) {
                    area += (it->points)[0].first * (it->points)[0].second;
                }
            }
            return area;
        }

        void init_vbuf() {
            const_iterator it = vtree_.begin();
            balance_minstep = 0;
            while (it != vtree_.end()) {
                vbuf_.push_back(it);
                it++;
                balance_minstep += 40;
            }
        }

        double count_init_temprature(double init_accept_rate) {
            int init_min_area = count_min_area();
            int post_min_area;
            int64_t total_drop;
            int N = 100;
            for (int i = 0; i < N; i++) {
                struct operation op = random_operation();
                struct operation op_final = check_valid_and_go(op);
                post_min_area = count_min_area();
                total_drop += abs(init_min_area - post_min_area);
                goto_neighbor(op_final);   //recover previous state
            }
            AURELIANO_PRINT_EXPRS(total_drop) << endl;
            AURELIANO_PRINT_EXPRS(init_accept_rate) << endl;
            return - total_drop / (100 * log(init_accept_rate));
        }

        int count_min_area() {
            // print_coord_list((std::prev(vbuf_in.end()))->points);
            int min_area = -1, curve_index = 0, sub_best_curve;
            for (auto &&e : vbuf_.back()->points) {
                if (e.first * e.second < min_area || min_area < 0) {
                    min_area = e.first * e.second;
                    best_curve = curve_index;
                }
                curve_index++;
            }
            if (min_area < best_solution || best_solution < 0) {
                best_solution = min_area;
                best_buf = vbuf_;
                best_curve = sub_best_curve;
                best_tree = vtree_;
            }
            return min_area;
        }

        void goto_neighbor(struct operation op) {
            if (op.type == operation_type::M2) {
                vtree_.invert_chain(vbuf_[op.target1]);
            } else {
                vtree_.swap_nodes(vbuf_[op.target1], vbuf_[op.target2]);
                std::swap(vbuf_[op.target1], vbuf_[op.target2]);
            }
        }

        //检查操作合法性，校正并执行操作
        struct operation check_valid_and_go(struct operation op) {
            if (op.type == operation_type::M1) {
                // std::cerr<< "step M1 " << op.target1 << endl;
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
                // std::cerr<< "step M2 " << op.target1 << endl;
                //如果是叶节点，非法
                while (vbuf_[op.target1]->type == combine_type::LEAF) {
                    op.target1 = rand() % (vbuf_.size() - 1);
                }
                vtree_.invert_chain(vbuf_[op.target1]);
            } else if (op.type == operation_type::M3) {
                // std::cerr<< "step M3 " << op.target1 << endl;
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
            // std::cerr<< "finished" << endl;
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
            std::ostream &os = std::cerr) {
            for (auto &&e : cont)
                cerr << "(" << e.first << "," << e.second << ") ";
            return os;
        }

        std::vector<const_iterator> vbuf_, best_buf;
        vctr_tree_type vtree_, best_tree;
        float temperature, cooldown_speed;
        int accept_under_currentT, total_under_currentT;
        float ending_temperature;
        int best_solution, best_curve, tot_block_area;
        int balance_minstep;
    };

}
