// test.cpp: testcases for slicing tree.
// Author: LYL
#include "sa.hpp"

#define BOOST_TEST_MODULE polish_test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <sstream>
#include <string>

#include "polish_tree.hpp"

using namespace std;
using namespace polish;

using combine_type = typename meta_polish_node::combine_type;
using dimension_type = typename meta_polish_node::dimension_type;
using tree_type = polish::polish_tree<>;
using vtree_type = polish::vectorized_polish_tree<>;

namespace {

    std::default_random_engine eng(std::random_device{}());
    vector<typename tree_type::value_type> buf;
    vector<typename vtree_type::value_type> vbuf;

    template<typename Tree>
    auto get_iter(Tree &&t, std::size_t k) {
        return std::next(t.begin(), k);
    }

    template<typename Tree>
    bool is_m3_valid(const Tree &t, uint32_t i) {
        if (i + 1 >= std::distance(t.begin(), t.end()))
            return false;
        auto p = get_iter(t, i), q = std::next(p);
        if ((p->type == combine_type::LEAF) == (q->type == combine_type::LEAF))
            return false;
        if (q->type == combine_type::LEAF)  // operator-leaf swap
            return true;
        // leaf-operator swap
        auto prefix_oprerators = 1 + std::count_if(t.begin(), q,
            [](const auto &e) { return e.type != combine_type::LEAF; });
        return 2 * prefix_oprerators < i + 1;
    }

    bool test_traversal(const tree_type &t) {
        buf.assign(t.begin(), t.end());
        return std::equal(buf.rbegin(), buf.rend(),
            std::make_reverse_iterator(t.end()),
            [](const auto &x, const auto &y) {
            return x.type == y.type && x.width == y.width
                && x.height == y.height;
        });
    }

    bool test_traversal(const vtree_type &t) {
        vbuf.assign(t.begin(), t.end());
        return std::equal(vbuf.rbegin(), vbuf.rend(),
            std::make_reverse_iterator(t.end()),
            [](const auto &x, const auto &y) {
            return x.type == y.type && x.points == y.points;
        });
    }

    template<typename Tree>
    bool test_normalized(const Tree &t) {
        return std::adjacent_find(t.begin(), t.end(),
            [](const auto &a, const auto &b) {
            return a.type != meta_polish_node::combine_type::LEAF
                && a.type == b.type;
        }) == t.end();
    }

    bool intersects(dimension_type lo0, dimension_type hi0,
        dimension_type lo1, dimension_type hi1) {
        return (lo0 < hi1) ^ (lo1 >= hi0);
    }

    bool intersects(const typename vtree_type::floorplan_entry &a,
        const typename vtree_type::floorplan_entry &b) {
        return intersects(get<0>(a), get<0>(a) + get<2>(a),
            get<0>(b), get<0>(b) + get<2>(b))
            && intersects(get<1>(a), get<1>(a) + get<3>(a),
                get<1>(b), get<1>(b) + get<3>(b));
    }

    template<typename FwdIt>
    bool check_intersection(FwdIt first, FwdIt last) {
        for (auto i = first; i != last; ++i)
            for (auto j = std::next(i); j != last; ++j)
                if (intersects(*i, *j))
                    return false;
        return true;
    }

    struct BasicFixture {
        BasicFixture() {
            yal::Module m;
            m.xpos.push_back(0);
            m.ypos.push_back(0);
            m.xpos.push_back(30);
            m.ypos.push_back(20);
            modules.assign(6, m);
            expr = {
                0, 1, expression::COMBINE_HORIZONTAL,
                2, 3, expression::COMBINE_VERTICAL, 4, 5,
                expression::COMBINE_VERTICAL, expression::COMBINE_HORIZONTAL,
                expression::COMBINE_VERTICAL
            };
        }

        std::vector<yal::Module> modules;
        std::vector<expression::polish_expression_type> expr;
    };

}

BOOST_AUTO_TEST_SUITE(test_polish)

BOOST_AUTO_TEST_CASE(test_ebo) {
    BOOST_TEST(sizeof(tree_type) == sizeof(void *));
}

using tree_types = boost::mpl::list<tree_type, vtree_type>;

BOOST_AUTO_TEST_CASE_TEMPLATE(m3_test, Tree, tree_types) {
    std::vector<yal::Module> modules(1);
    std::vector<expression::polish_expression_type> expr;
    uniform_int_distribution<size_t> rand_size(3, 512),
        rand_times(64, 256);

    for (size_t cnt = 0; cnt != 8; ++cnt) {
        size_t sz = rand_size(eng);
        if (!(sz & 1))
            ++sz;
        expr.assign(sz, 0);
        if (cnt & 1) {
            int state = -1;
            for (auto &e : expr) {
                if (state == 1) {
                    e = expression::COMBINE_VERTICAL;
                } else if (state == 3) {
                    e = expression::COMBINE_HORIZONTAL;
                }
                if (++state == 4)
                    state = 0;
            }
        } else {
            for (auto i = sz / 2 + 1; i != sz; ++i) {
                expr[i] = (i & 1) ? expression::COMBINE_HORIZONTAL
                    : expression::COMBINE_VERTICAL;
            }
        }

        Tree t;
        BOOST_TEST(t.construct(modules, expr));
        BOOST_TEST(test_traversal(t));
        BOOST_TEST(t.check_integrity());

        uniform_int_distribution<typename Tree::size_type>
            rand(0, std::distance(t.begin(), t.end()) - 2);

        size_t threshold = 1 << 16, times = rand_times(eng);
        while (threshold-- && times) {
            auto idx = rand(eng);
            auto i = get_iter(t, idx), j = get_iter(t, idx + 1);
            BOOST_TEST((i == std::prev(j)));
            BOOST_TEST((j == std::next(i)));
            if (i->type == combine_type::LEAF
                ^ j->type == combine_type::LEAF) {
                bool valid = is_m3_valid(t, idx);
                if (!t.swap_nodes(i, j)) {
                    BOOST_TEST(!valid);
                } else {
                    BOOST_TEST(valid);
                    BOOST_TEST(test_traversal(t));
                    BOOST_TEST(t.check_integrity());
                    auto t2 = t;
                    auto ret = t2.swap_nodes(get_iter(t2, idx), get_iter(t2, idx + 1));
                    BOOST_TEST(ret);
                    BOOST_TEST(test_traversal(t2));
                    BOOST_TEST(t2.check_integrity());
                    --times;
                }
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(test_tree_rotate_leaf, BasicFixture) {
    tree_type tree;
    tree.construct(modules, expr);

    auto root = std::prev(tree.end());
    auto w = root->width, h = root->height;
    for (auto i = tree.begin(); i != tree.end(); ++i) {
        if (i->type != combine_type::LEAF) {
            BOOST_TEST((!tree.rotate_leaf(i)));
            continue;
        }
        BOOST_TEST(tree.rotate_leaf(i));
        BOOST_TEST(tree.rotate_leaf(i));
        BOOST_TEST((root == std::prev(tree.end())));
        BOOST_TEST((w == root->width && h == root->height));
    }
    BOOST_TEST((!tree.rotate_leaf(tree.end())));
}

BOOST_AUTO_TEST_CASE(test_curve) {
    std::allocator<typename polish::meta_polish_node::coord_type> alloc;
    polish::basic_vectorized_polish_node<> p(combine_type::VERTICAL, alloc),
        lc(combine_type::LEAF, alloc), rc(combine_type::LEAF, alloc);
    lc.points = { {1, 5}, {3, 2}, {5, 0} };
    rc.points = { {2, 3}, {4, 1}, {5, 0} };
    combine_type types[2] = { combine_type::VERTICAL, combine_type::HORIZONTAL };
    vector<typename tree_type::coord_type> expected[2];
    expected[0] = { {2, 8}, {3, 5}, {4, 3}, {5, 0} };
    expected[1] = { {3, 5}, {5, 3}, {7, 2}, {9, 1}, {10, 0} };

    for (size_t i = 0; i != 2; ++i) {
        p.type = types[i];
        p.count_area(lc, rc);
        BOOST_TEST((p.points == expected[i]));
    }
}

BOOST_FIXTURE_TEST_CASE(test_tree_floorplan, BasicFixture) {
    tree_type tree;
    tree.construct(modules, expr);
    auto root = std::prev(tree.end());
    vector<typename tree_type::floorplan_entry> result, expected;
    expected = { {0, 0}, {30, 0}, {0, 20}, {0, 40}, {30, 20}, {30, 40} };
    tree.floorplan(std::back_inserter(result));
    BOOST_TEST((result == expected));
}

BOOST_FIXTURE_TEST_CASE(test_vtree_floorplan, BasicFixture) {
    vtree_type vtree;
    vtree.construct(modules, expr);
    auto root = std::prev(vtree.end());
    decltype(root->points) expected_points = { {40, 90}, {60, 60} };
    BOOST_TEST((root->points == expected_points));
    vector<typename vtree_type::floorplan_entry> result;
    for (size_t i = 0; i != root->points.size(); ++i) {
        result.clear();
        vtree.floorplan(i, std::back_inserter(result));
        BOOST_TEST(check_intersection(result.cbegin(), result.cend()));
    }
}

BOOST_FIXTURE_TEST_CASE(test_tree_random_construct, BasicFixture) {
    tree_type tree;
    vector<size_t> indices(modules.size());
    iota(indices.begin(), indices.end(), 0);
    for (size_t i = 0; i != 16; ++i) {
        BOOST_TEST((tree.construct(modules.begin(),
            indices.begin(), indices.end(), eng)));
        BOOST_TEST((tree.check_integrity()));
        BOOST_TEST((test_traversal(tree)));
        BOOST_TEST((std::distance(tree.begin(), tree.end())
            == 2 * modules.size() - 1));
        BOOST_TEST((test_normalized(tree)));
    }
}

BOOST_FIXTURE_TEST_CASE(test_tree_shuffle, BasicFixture) {
    tree_type tree;
    tree.construct(modules, expr);
    for (size_t i = 0; i != 16; ++i) {
        tree.shuffle(eng);
        BOOST_TEST((tree.check_integrity()));
        BOOST_TEST((test_traversal(tree)));
        BOOST_TEST((std::distance(tree.begin(),
            tree.end()) == 2 * modules.size() - 1));
        BOOST_TEST((test_normalized(tree)));
    }
}

BOOST_FIXTURE_TEST_CASE(test_tree_assign, BasicFixture) {
    tree_type t;
    vector<typename tree_type::const_iterator> iters;
    for (size_t cnt = 0; cnt != 2; ++cnt) {
        iters.clear();
        t.clear();
        if (cnt & 1)
            t.construct(modules, expr);
        for (auto i = t.begin(); i != t.end(); ++i)
            iters.push_back(i);
        tree_type t2;
        t2.assign(iters.cbegin(), iters.cend());
        BOOST_TEST((t2.check_integrity()));
        BOOST_TEST((test_traversal(t2)));
        ostringstream s1, s2;
        t.print_tree(s1);
        t2.print_tree(s2);
        BOOST_TEST((s1.str() == s2.str()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
