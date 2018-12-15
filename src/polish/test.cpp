// test.cpp: testcases for slicing tree.
// Author: LYL
#include "sa.hpp"

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
