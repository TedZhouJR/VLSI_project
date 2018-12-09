//  main.cpp
//  simulate anneal

#include <iostream>
using namespace std;
#include "polish_tree.hpp"

void test() {
	polish_tree* tree = new polish_tree();
	polish_node* node0 = new polish_node('+', 0.0, 0.0);
	polish_node* node1 = new polish_node('*', 2.0, 3.0);
	polish_node* node2 = new polish_node('*', 5.0, 4.0);
	tree->root = node0;
	node0->lc = node1;
	node0->rc = node2;
	node1->parent = node2->parent = node0;
	tree->count_area(tree->root);
	cout << tree->root->area << endl;
}

int main() {
	test();
	return 0;
}
