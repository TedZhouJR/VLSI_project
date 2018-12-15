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

int main(int argc, char **argv) {
    cout << "Start simulate anneal..." << endl;
    SA sa(0.7, 30, 0.1, 40, 10);
    while(!sa.reach_end()) {
        while(!sa.reach_balance()) {
            sa.take_step();
        }
        sa.cool_down_by_speed();
        // cout << "cool down" << endl;
    }
    sa.print();
    cout << "passed!" << endl;

    // test();
    return 0;
}
