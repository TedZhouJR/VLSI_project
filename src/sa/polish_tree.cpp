//  polish_tree.cpp
//  sa

#include "polish_tree.hpp"

polish_tree::polish_tree() {
	this->root = NULL;
}

void polish_tree::count_area(polish_node* node) {	//后序遍历计算面积
	if (node->lc != NULL) {
		count_area(node->lc);
	}
	if (node->rc != NULL) {
		count_area(node->rc);
	}
	//先抵达
	node->count_area();
}
