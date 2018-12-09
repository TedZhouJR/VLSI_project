//  polish_tree.hpp
//  sa

#ifndef polish_tree_hpp
#define polish_tree_hpp

#include <stdio.h>
#include "polish_node.hpp"

class polish_tree {
public:
	polish_node* root;
	polish_tree();
	void count_area(polish_node* node);
};

#endif /* polish_tree_hpp */
