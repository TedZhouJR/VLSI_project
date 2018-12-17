// test.cpp: testcases for slicing tree.
// Author: LYL
#include "sa.hpp"

void test() {
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
}

void load_tree(polish::vectorized_polish_tree<>* vtree_) {
    std::vector<expression::polish_expression_type> expr_;
    std::vector<yal::Module> modules_;
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
    vtree_->construct(modules_, expr_);
}

int main(int argc, char **argv) {
    cout << "Start simulate anneal..." << endl;
    polish::vectorized_polish_tree<> vtree_;
    load_tree(&vtree_);
    float init_accept_rate = 0.9, cooldown_speed = 0.05, ending_temperature = 20;
    int min_acc_to_balance = vtree_.size();    //TODO decided by amount of nodes
    SA sa(&vtree_, init_accept_rate, cooldown_speed, min_acc_to_balance, ending_temperature);
    while(!sa.reach_end()) {
        while(!sa.reach_balance()) {
            sa.take_step();
        }
        sa.cool_down_by_ratio();
        // cout << "cool down" << endl;
    }
    sa.print();
    cout << "passed, result is : " << sa.current_solution() << endl;

    return 0;
}
