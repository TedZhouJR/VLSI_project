//  main.cpp
//  simulate anneal

#include <iostream>
#include "sa.hpp"
using namespace std;

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
    SA sa(&vtree_, init_accept_rate, cooldown_speed, ending_temperature);
    while(!sa.reach_end()) {
        while(!sa.reach_balance()) {
            sa.take_step();
        }
        sa.cool_down_by_ratio();
        // cout << "cool down" << endl;
    }
    sa.print_current_solution();

    return 0;
}
