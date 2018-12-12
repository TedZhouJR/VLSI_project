//  main.cpp
//  simulate anneal

#include <iomanip>
#include <iostream>
#include <numeric>

#include "polish_tree.hpp"

using namespace std;
using namespace polish;

int main() {
    cout << "hello" << endl;

    std::vector<yal::Module> modules;
    yal::Module m;
    m.xpos.push_back(0);
    m.ypos.push_back(0);
    m.xpos.push_back(100);
    m.ypos.push_back(50);

    for (int i = 0; i != 5; ++i) {
        modules.push_back(m);
        if (i & 1) {
            m.xpos.back() *= 2;
        } else {
            m.ypos.back() *= 2;
        }
    }

    cout << endl << "Modules: (width, height)" << endl;
    for (const yal::Module &m : modules)
        cout << "(" << m.xspan() << ", " << m.yspan() << ") ";
    cout << endl << endl;

    std::vector<expression::polish_expression_type> expr(2 * modules.size() - 1);
    iota(expr.begin(), expr.begin() + modules.size(), 0);
    for (auto i = modules.size(); i != expr.size(); ++i)
        expr[i] = (i & 1) ? expression::HORIZONTAL_COMBINE : expression::VERTICAL_COMBINE;

    cout << "Expression:" << endl;
    for (auto e : expr) {
        switch (e) {
        case expression::HORIZONTAL_COMBINE:
            cout << polish_node::HORIZONTAL_COMBINE;
            break;
        case expression::VERTICAL_COMBINE:
            cout << polish_node::VERTICAL_COMBINE;
            break;
        default:
            cout << e;
        }
        cout << " ";
    }
    cout << endl << endl;
        
    // Construct tree with a list of modules and a polish expression.
    polish::polish_tree<> t;
    t.construct(modules, expr);
    cout << "Nodes (" << t.size() << "): (type, width, height, size)" << endl;
    for (typename decltype(t)::size_type i = 0; i != t.size(); ++i) {
        auto p = t.get(i);
        cout << p->combine_type << "  " << setw(3) << p->width 
            << " " << setw(3) << p->height << " " << setw(2) << p->size << endl;
    }

	return 0;
}
