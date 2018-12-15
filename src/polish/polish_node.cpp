//  polish_node.cpp

#include <algorithm>
#include <cassert>
#include <iostream>

#include "polish_node.hpp"

using namespace std;
using namespace polish;
using namespace polish::detail;

std::ostream & polish::operator<<(std::ostream &os, const meta_polish_node &n) {
    switch (n.type) {
    case meta_polish_node::combine_type::HORIZONTAL:
        os << "*";
        break;
    case meta_polish_node::combine_type::VERTICAL:
        os << "+";
        break;
    case meta_polish_node::combine_type::LEAF:
        os << "[]";
        break;
        assert(false);
    }
    return os;
}

std::ostream & polish::operator<<(std::ostream &os, const basic_polish_node &n) {
    if (n.type == basic_polish_node::combine_type::LEAF)
        os << "(" << n.width << "," << n.height << ")";
    else
        os << (n.type == basic_polish_node::combine_type::HORIZONTAL ? "*" : "+");
    return os;
}

const typename tree_node_base::self * tree_node_base::prev() const noexcept {
    if (rc_)
        return rc_;
    if (lc_)
        return lc_;   // iff header_
    const self *t = this, *p = parent_;
    while (t == p->lc_) {
        t = p;
        p = p->parent_;
    }
    return p->lc_;
}

const typename tree_node_base::self * tree_node_base::next() const noexcept {
    const self *p = parent_;
    if (this == p->rc_ || p->is_header()) // p has either 0 or 2 children
        return p;   // possibly header_
    p = p->rc_;      // not null
    while (p->lc_)   // p has either 0 or 2 children
        p = p->lc_;
    return p;
}

