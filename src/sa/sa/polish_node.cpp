//  polish_node.cpp
//  sa
#include <iostream>
#include <algorithm>
#include "polish_node.hpp"
using namespace std;

polish_node::polish_node(char combine_type_in, float height_in, float weight_in) {
	this->combine_type = combine_type_in;
	this->height = height_in;
	this->width = weight_in;
	this->lc = NULL;
	this->rc = NULL;
	this->parent = NULL;
	this->counted = false;
	this->area = height_in * weight_in;
}

void polish_node::count_area() {
	if(this->lc == NULL || this-> rc == NULL) {
		this->counted = true;
		this->area = this->height * this->width;
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
	this->counted = true;
	this->area = this->height * this->width;
}
