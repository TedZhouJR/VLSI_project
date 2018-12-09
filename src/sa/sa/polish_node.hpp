//  polish_node.hpp
//  sa

#ifndef polish_node_hpp
#define polish_node_hpp

#include <stdio.h>

class polish_node {
public:
	char combine_type;	//*(左右结合) or +(上下结合)
	float height;
	float width;
	bool counted;
	float area;
	polish_node* lc;	//left child
	polish_node* rc;	//right child
	polish_node* parent;	//parent node
	
	polish_node(char combine_type_in, float height_in, float weight_in);
	
	void count_area();
};

#endif /* polish_node_hpp */
