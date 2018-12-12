//  polish_node.cpp
//  sa
#include <iostream>
#include <algorithm>
#include "polish_node.hpp"
using namespace std;
using namespace polish;

polish_node::polish_node(char combine_type_in, 
    dimension_type height_in, dimension_type width_in) {
	this->combine_type = combine_type_in;
	this->height = height_in;
	this->width = width_in;
	this->lc = NULL;
	this->rc = NULL;
	this->parent = NULL;
	//this->counted = false;
    this->size = 1;
    this->area = static_cast<area_type>(height_in)
        * static_cast<area_type>(width_in); // 当前cast无用
}   

void polish_node::count_area() {
	if(this->lc == NULL || this-> rc == NULL) {
		//this->counted = true;
		this->area = static_cast<area_type>(height)
            * static_cast<area_type>(width);
		return;
	} else {
		if (this->combine_type == '+') {	//上下结合
			this->height = this->lc->height + this->rc->height;
			this->width = max(this->lc->width, this->rc->width);
		} else if (this->combine_type == '*') {	//左右结合
			this->height = max(this->lc->height, this->rc->height);
			this->width = this->lc->width + this->rc->width;
		} else {
			cout << "combine type not found!" << endl;
			exit(1);
		}
	}
	//this->counted = true;
	this->area = static_cast<area_type>(height)
        * static_cast<area_type>(width);
}
