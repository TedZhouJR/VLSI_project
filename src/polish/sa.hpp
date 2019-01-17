// test.cpp: testcases for slicing tree.
// Author: LYL

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>

#include <boost/pool/pool_alloc.hpp>

#include "polish_tree.hpp"

namespace polish {
    namespace v1 {

        class SA {
            enum class operation_type {
                M1, M2, M3
            };

            struct operation {
                operation_type type;
                int target1;
                int target2;
            };

        public:
            using combine_type = typename basic_polish_node::combine_type;
            using tree_type = polish::polish_tree<>;
            using vctr_tree_type = polish::vectorized_polish_tree<
                boost::fast_pool_allocator<polish::basic_vectorized_polish_node<
                boost::fast_pool_allocator<polish::meta_polish_node::coord_type>>>>;
            using const_iterator = typename vctr_tree_type::const_iterator;

            SA(vctr_tree_type* vtree_in, int best_curve_in, double init_accept_rate, double cooldown_speed_in, double ending_temperature_in)
            {
                using namespace std;
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
                std::cerr << "minimum area is " << best_solution << std::endl;
                std::cerr << "utility is " << (double)tot_block_area / best_solution << std::endl;
                return (double)tot_block_area / best_solution;
            }

            vctr_tree_type show_best_tree() {
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
                os << std::endl;
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
                AURELIANO_PRINT_EXPRS(total_drop) << std::endl;
                AURELIANO_PRINT_EXPRS(init_accept_rate) << std::endl;
                return -total_drop / (100 * log(init_accept_rate));
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
                    os << "(" << e.first << "," << e.second << ") ";
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

    }   // namespace v1

    inline namespace v2 {
        namespace detail {

            enum class OperationType {
                M1, M2, M3, M4
            };

            struct Operation {
                OperationType type;
                int target1;
                int target2;
            };

            template<typename T>
            class SABase;

            template<typename Alloc>
            class SABase<polish::vectorized_polish_tree<Alloc>> {
            protected:
                using area_type = std::int64_t;
                using tree_type = polish::vectorized_polish_tree<Alloc>;
                using combine_type = typename tree_type::combine_type;
                using const_iterator = typename tree_type::const_iterator;

            public:
                static std::size_t get_best_point(const tree_type &tree) {
                    assert(!tree.empty());
                    return count_min_area_impl(tree.root()).second;
                }
            
            protected:
                template<typename Eng>
                static OperationType random_operation(Eng &&eng, std::size_t sz) {
                    if (sz <= 1)
                        return OperationType::M1;

                    int x = std::uniform_int_distribution<>(0, 2)(eng);
                    switch (x) {
                    case 0:
                        return OperationType::M1;
                    case 1:
                        return OperationType::M2;
                    case 2:
                        return OperationType::M3;
                    default:
                        assert(false);
                        return OperationType::M1;
                    }
                }

                static bool conduct_m4(tree_type &t, const_iterator it) {
                    assert(false);
                    return false;
                }

                static area_type count_min_area(const_iterator root) noexcept {
                    return count_min_area_impl(root).first;
                }

                static int compute_balance_minstep(std::size_t tree_size) noexcept {
                    return 20 * tree_size;
                }

            private:
                static std::pair<area_type, std::size_t> 
                    count_min_area_impl(const_iterator root) noexcept {
                    area_type min_area = std::numeric_limits<area_type>::max();
                    std::size_t curve_index = 0, sub_best_curve = -1;
                    for (auto &&e : root->points) {
                        if (e.first * e.second < min_area) {
                            min_area = e.first * e.second;
                            sub_best_curve = curve_index;   // There was a bug in v1!!
                        }
                        curve_index++;
                    }
                    return { min_area, sub_best_curve };
                }
            };

            template<typename Alloc>
            class SABase<polish::polish_tree<Alloc>> {
            protected:
                using area_type = std::int64_t;
                using tree_type = polish::polish_tree<Alloc>;
                using combine_type = typename tree_type::combine_type;
                using const_iterator = typename tree_type::const_iterator;

                template<typename Eng>
                static OperationType random_operation(Eng &&eng, std::size_t sz) {
                    if (sz <= 1)
                        return std::uniform_int_distribution<>(0, 1)(eng) ? 
                        OperationType::M1 : OperationType::M4;

                    int x = std::uniform_int_distribution<>(0, 3)(eng);
                    switch (x) {
                    case 0:
                        return OperationType::M1;
                    case 1:
                        return OperationType::M2;
                    case 2:
                        return OperationType::M3;
                    case 3:
                        return OperationType::M4;
                    default:
                        assert(false);
                        return OperationType::M1;
                    }
                }

                static bool conduct_m4(tree_type &t, const_iterator it) {
                    return t.rotate_leaf(it);
                }

                static area_type count_min_area(const_iterator root) noexcept {
                    return root->width * root->height;
                }

                static int compute_balance_minstep(std::size_t tree_size) noexcept {
                    return 128 * tree_size;
                }
            };

        }

        template<typename Tree>
        class SA : public detail::SABase<Tree> {
            using base = detail::SABase<Tree>;
            using operation_type = detail::OperationType;
            using operation = detail::Operation;
            using typename base::area_type;
            using typename base::combine_type;
            using typename base::const_iterator;

        public:
            using tree_type = Tree;

            template<typename Eng>
            SA(const tree_type &vtree_in, 
                double init_accept_rate, double cooldown_speed_in,
                double ending_temperature_in, Eng &&eng,
                std::ostream &out = std::cerr) :
                tree(vtree_in), best_tree(vtree_in), os(&out), 
                cooldown_speed(cooldown_speed_in), 
                ending_temperature(ending_temperature_in),
                accept_under_currentT(0), total_under_currentT(0),
                best_solution(std::numeric_limits<area_type>::max()) {
                init_expr();
                temperature = count_init_temprature(init_accept_rate, eng);
                (*os) << "init temperature " << temperature << std::endl;
                balance_minstep = base::compute_balance_minstep(
                    (1 + expr.size()) / 2);
            }

            SA(const SA &) = delete;
            SA &operator=(const SA &) = delete;

            template<typename Eng>
            void take_step(Eng &&eng) {
                area_type pre_min_area, post_min_area;
                pre_min_area = count_min_area();
                operation_type op = random_operation(eng);
                operation op_final = check_valid_and_go(op, eng);
                post_min_area = count_min_area();
                std::uniform_real_distribution<> rand_double;
                if (pre_min_area <= post_min_area) { //probably accept
                    double acc_rate = exp((pre_min_area - post_min_area) / temperature);
                    if (rand_double(eng) <= acc_rate) {
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
            bool reach_balance() const noexcept {
                return accept_under_currentT > balance_minstep;
            }

            void cool_down_by_ratio() noexcept {
                temperature = temperature * (1 - cooldown_speed);
                accept_under_currentT = total_under_currentT = 0;
            }

            bool reach_end() const noexcept {
                return temperature <= ending_temperature;
            }

            void print() const {
                for (const auto &e : expr)
                    (*os) << e << " ";
                (*os) << std::endl;
                tree.print_tree(*os, 2);
            }

            void print_statistics() const {
                (*os) << "minimum area is " << best_solution << std::endl;
                (*os) << "utility is " << static_cast<double>(count_tot_block_area())
                    / best_solution << std::endl;
            }

            area_type get_best_area() const noexcept {
                return best_solution;
            }

            const tree_type &get_best_tree() const noexcept {
                return best_tree;
            }

        private:
            area_type count_tot_block_area() const {
                area_type area = 0;
                for (auto it : expr) {
                    if (it->type == combine_type::LEAF) {
                        area += base::count_min_area(it);
                    }
                }
                return area;
            }

            void init_expr() {
                const_iterator it = tree.begin();
                while (it != tree.end()) {
                    expr.push_back(it);
                    it++;
                }
            }

            template<typename Eng>
            double count_init_temprature(double init_accept_rate, Eng &&eng) {
                using namespace std;
                area_type init_min_area = count_min_area();
                int64_t total_drop = 0;
                int N = 100;
                for (int i = 0; i < N; i++) {
                    operation_type op = random_operation(eng);
                    operation op_final = check_valid_and_go(op, eng);
                    area_type post_min_area = count_min_area();
                    total_drop += abs(init_min_area - post_min_area);
                    goto_neighbor(op_final);   //recover previous state
                }
                return -total_drop / (100 * log(init_accept_rate));
            }

            area_type count_min_area() {
                area_type min_area = base::count_min_area(expr.back());
                if (min_area < best_solution) {
                    best_solution = min_area;
                    best_tree = tree;
                }
                return min_area;
            }

            void goto_neighbor(const operation &op) {
                using std::swap;
                if (op.type == operation_type::M2) {
                    tree.invert_chain(expr[op.target1]);
                } else if (op.type == operation_type::M4) {
                    base::conduct_m4(tree, expr[op.target1]);
                } else {
                    tree.swap_nodes(expr[op.target1], expr[op.target2]);
                    swap(expr[op.target1], expr[op.target2]);
                }
            }

            //检查操作合法性，校正并执行操作
            template<typename Eng>
            operation check_valid_and_go(operation_type tp, Eng &&eng) {
                using std::swap;
                operation op;
                op.type = tp;
                std::uniform_int_distribution<> rand_int(0, expr.size() - 2);

                switch (op.type) {
                case operation_type::M1: {
                    op.target1 = -1;
                    while (op.target1 < 0) {
                        op.target2 = rand_int(eng);
                        //如果是非叶节点，非法
                        if (expr[op.target2]->type == combine_type::LEAF) {
                            //找左边的相邻叶节点，找不到则非法
                            for (op.target1 = op.target2 - 1; op.target1 >= 0; op.target1--) {
                                if (expr[op.target1]->type == combine_type::LEAF)
                                    break;
                            }
                        }
                    }
                    tree.swap_nodes(expr[op.target1], expr[op.target2]);
                    swap(expr[op.target1], expr[op.target2]);
                    break;
                } case operation_type::M2: {
                    //如果是叶节点，非法
                    do {
                        op.target1 = rand_int(eng);
                    } while (expr[op.target1]->type == combine_type::LEAF);
                    tree.invert_chain(expr[op.target1]);
                    break;
                } case operation_type::M3: {
                    bool valid = false;
                    while (!valid) {
                        op.target1 = rand_int(eng);
                        op.target2 = op.target1 + 1;
                        valid = tree.swap_nodes(expr[op.target1], expr[op.target2]);
                    }
                    swap(expr[op.target1], expr[op.target2]);
                    break;
                } case operation_type::M4: {
                    do {
                        op.target1 = rand_int(eng);
                    } while (expr[op.target1]->type != combine_type::LEAF);
                    base::conduct_m4(tree, expr[op.target1]);
                    break;
                } default: {
                    assert(false);
                }
                }
                return op;
            }

            template<typename Eng>
            operation_type random_operation(Eng &&eng) const {
                return base::random_operation(eng, (expr.size() + 1) >> 1);
            }

            std::vector<const_iterator> expr;
            tree_type tree, best_tree;
            double temperature, cooldown_speed, ending_temperature;
            int accept_under_currentT, total_under_currentT, balance_minstep;
            area_type best_solution;
            std::ostream *os;
        };

    }   // namespace v2
}   // namespace polish
