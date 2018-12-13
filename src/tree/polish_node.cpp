//  polish_node.cpp
//  sa

#include <algorithm>
#include <cassert>
#include <iostream>

#include "polish_node.hpp"

using namespace std;
using namespace polish;

std::ostream & polish::operator<<(std::ostream &os, const basic_polish_node &n) {
    if (n.type == basic_polish_node::combine_type::LEAF)
        os << "(" << n.width << "," << n.height << ")";
    else
        os << (n.type == basic_polish_node::combine_type::HORIZONTAL ? "*" : "+");
    return os;
}

void polish_node::count_area() {
    assert(!lc == !rc);
    if (lc) {
        assert(type == combine_type::HORIZONTAL || type == combine_type::VERTICAL);
        if (this->type == combine_type::VERTICAL) {	//上下结合
            this->height = this->lc->height + this->rc->height;
            this->width = max(this->lc->width, this->rc->width);
        } else {	//左右结合
            this->height = max(this->lc->height, this->rc->height);
            this->width = this->lc->width + this->rc->width;
        }
    }
}

bool polish_node::check_area() const {
    assert(!lc == !rc);
    if (is_leaf())
        return true;
    assert(type == combine_type::HORIZONTAL || type == combine_type::VERTICAL);
    if (this->type == combine_type::VERTICAL) {	//上下结合
        return this->height == this->lc->height + this->rc->height
            && this->width == max(this->lc->width, this->rc->width);
    } else {	//左右结合
        return this->height == max(this->lc->height, this->rc->height)
            && this->width == this->lc->width + this->rc->width;
    }
}

const typename polish_node::self * polish_node::prev() const {
    if (rc)
        return rc;
    if (lc)
        return lc;   // iff header_
    const self *t = this, *p = parent;
    while (t == p->lc) {
        t = p;
        p = p->parent;
    }
    return p->lc;
}

const typename polish_node::self * polish_node::next() const {
    const self *p = parent;
    if (this == p->rc || p->is_header()) // p has either 0 or 2 children
        return p;   // possibly header_
    p = p->rc;      // not null
    while (p->lc)   // p has either 0 or 2 children
        p = p->lc;
    return p;
}

std::ostream & polish_node::print_tree(std::ostream &os, int offset, 
    int ident, char fill, bool print_size) const {
    if (rc)
        rc->print_tree(os, offset + ident, ident, fill);
    for (int i = 0; i < offset; ++i)
        os << fill;
    if (this->type != combine_type::LEAF) {
        os << (this->type == combine_type::HORIZONTAL ? "*" : "+");
        if (print_size)
            os << " (" << this->width << "," << this->height << ")";
        os << "\n";
    } else {
        os << "(" << this->width << "," << this->height << ")\n";
    }
    if (lc)
        lc->print_tree(os, offset + ident, ident, fill);
    return os;
}

