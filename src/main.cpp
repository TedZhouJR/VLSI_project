//  main.cpp
//  simulate anneal

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/pool/pool_alloc.hpp>

#include "timeit.h"
#include "toolbox.h"
#include "layout.h"
#include "pack_generator.h"
#include "sa_packer.h"
#include "verification.h"
#include "interpreter.h"
#include "sa.hpp"

using namespace std;
using namespace seqpair;
namespace po = boost::program_options;

namespace {
    using dimension_type = typename polish::basic_polish_node::dimension_type;
    using vtree_type = typename polish::vectorized_polish_tree<>;
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

    template<typename Generator, typename Alloc, typename FwdIt>
    void run_packer(SaPacker<Generator> &packer, Layout<Alloc> &layout,
        FwdIt first_line, FwdIt last_line, ostream &out,
        int verbose_level) {
        using namespace seqpair::verification;
        using change_t = PackGeneratorBase::change_t;

        cerr << packer.options();

        // Change distribution and runtime allocator
        // Note: maybe pool_options can be specified
        PackGeneratorBase::default_change_distribution chg_dist;

        double cost = 0;
        auto runtime = aureliano::timeit([&] {
            cost = packer(layout, first_line, last_line,
                chg_dist, verbose_level);
        });

        cerr << "\n";
        cerr << "Runtime: " <<
            chrono::duration_cast<chrono::milliseconds>(runtime).count() <<
            "ms" << "\n";
        auto sum_rect_areas = layout.sum_conponent_areas();
        cerr << "Sum of rectangle areas: " << sum_rect_areas << "\n";
        auto sln_area = layout.get_area();
        {
            using namespace seqpair::io;
            cerr << "Area: " << sln_area.first * sln_area.second <<
                " " << sln_area << "\n";
        }
        cerr << "Utilization: " << 1.0 * sum_rect_areas /
            (sln_area.first * sln_area.second) << "\n";
        auto wirelen = sum_manhattan_distances(layout, first_line, last_line);
        cerr << "Wirelength: " << wirelen << "\n";
        cerr << "Cost: " << cost << "\n";

        auto alpha = packer.energy_function().alpha;
        if (abs((alpha * sln_area.first * sln_area.second + (1 - alpha) * wirelen) / cost - 1) > 1e-5)
            cerr << "Wrong answer: incorrect cost." << "\n";
        else if (has_intersection(layout))
            cerr << "Wrong answer: layout contains intersections." << "\n";
        else
            cerr << "Answer accepted.\n";
        cerr << "\n";

        using format_policy = typename Layout<Alloc>::format_policy;
        out << layout.format(format_policy::no_delim);
    }

    unordered_map<string, size_t> make_modulename_index(const yal::Interpreter &i) {
        unordered_map<string, size_t> map;
        map.reserve(i.modules().size());
        size_t cnt = 0;
        for (const yal::Module &m : i.modules()) {
            map.emplace(m.name, cnt++);
        }
        return map;
    }

    void run_vectorized_polish_tree(const yal::Interpreter &interpreter) {
        cout << "Start simulate anneal..." << endl;
        polish::vectorized_polish_tree<> vtree;
        std::vector<typename polish::vectorized_polish_tree<>::floorplan_entry> result;
        auto module_index = interpreter.make_module_index();
        default_random_engine eng;  // (random_device{}()); // TODO: turn this on later
        vtree.construct(interpreter.modules().cbegin(),
            module_index.cbegin(), module_index.cend(), eng);
        cout << distance(vtree.begin(), vtree.end()) << endl;
        if (!vtree.check_integrity())
            cout << "Boom!" << endl;
        double init_accept_rate = 0.95, cooldown_speed = 0.01, ending_temperature = 20;
        int utility_stable = 0, pre_utility = 0, utility, best_curve;
        while (utility_stable < 1) {
            SA sa(&vtree, best_curve, init_accept_rate, cooldown_speed, ending_temperature);
            while (!sa.reach_end()) {
                while (!sa.reach_balance()) {
                    sa.take_step();
                }
                sa.cool_down_by_ratio();
                // cout << "cool down" << endl;
            }
            utility = sa.print_current_solution();
            if (pre_utility == utility) {
                utility_stable++;
            } else {
                utility_stable = 0;
            }
            vtree = sa.show_best_tree();
            best_curve = sa.show_best_curve();
            pre_utility = utility;
        }
        vtree.floorplan(best_curve, back_inserter(result));
        for (auto &&e: result) {
            std::cout << std::get<0>(e) << " " << std::get<1>(e) << " " << std::get<2>(e) << " " << std::get<3>(e) << std::endl;
        }
        std::cout << "check_integrity is " << vtree.check_integrity() << endl;
        if (!check_intersection(result.begin(), result.end()))
            std::cout << "intersections error!!" << std::endl;
        //sa.print();
        //cout << "passed, result is : " << sa.current_solution() << endl;
    }

}

int main(int argc, char **argv) {
    po::options_description desc("Options");
    desc.add_options()
        ("help,h",
            "show help message")
        ("input,i", po::value< vector<string> >(),
            "input YAL file (default cin)")
        ("output,o", po::value< vector<string> >(),
            "output placement file (default cout)")
        ("option,O", po::value< vector<string> >(),
            "option file for lcs/dag")
        ("method,m", po::value< vector<string> >(),
            "method (polish-v/lcs/dag, default polish-v)")
        ("verbose,v", po::value<int>()->default_value(1)->implicit_value(2),
            "verbose level (0-2)")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    try {
        string method = "polish-v";
        if (vm.count("method")) {
            method = vm["method"].as<vector<string>>().back();
            for (auto &e : method)
                e = tolower(e);
            if (method != "lcs" && method != "dag" && method != "polish-v")
                throw runtime_error("Unrecognized method: " + method);
        }

        yal::Interpreter interpreter;
        ifstream fin;
        if (vm.count("input")) {
            auto &&filename = vm["input"].as<vector<string>>().back();
            cerr << "Input stream: " << filename << endl;
            fin.open(filename);
            if (!fin.is_open())
                throw runtime_error("Cannot open file");
            interpreter.switch_input_stream(fin);
        } else {
            cerr << "Input stream: cin" << endl;
        }

        interpreter.parse();

        if (method == "polish-v") {
            cout << "Method: polish-v" << endl;
            run_vectorized_polish_tree(interpreter);

        } else {
            // LCS or DAG
            unordered_map<string, size_t> name_map = make_modulename_index(interpreter);
            Layout<> layout;
            for (const auto &e : interpreter.parent_module().network) {
                string name = yal::ParentModule::get_module_name(e);
                auto it = name_map.find(name);
                if (it == name_map.end())
                    throw runtime_error("Invalid module name: " + name);
                const yal::Module &m = interpreter.modules()[it->second];
                layout.push(m.xspan(), m.yspan());
            }

            SaPackerBase::options_t opts;
            if (vm.count("option")) {
                ifstream in(vm["option"].as<vector<string>>().back());
                if (!in.is_open())
                    throw runtime_error("Cannot open file");
                in >> opts; // Params
            } else {
                opts.simulaions_per_temperature
                    = max(30 * layout.size(), static_cast<size_t>(1024));
            }

            ostream *out = &cout;
            ofstream fout;
            if (vm.count("output")) {
                fout.open(vm["output"].as<vector<string>>().back());
                out = &fout;
            }

            int verbose_level = vm["verbose"].as<int>();

            vector<pair<size_t, size_t>> nets;

            cerr << "Rectangles: " << layout.size() << "\n";
            SaPackerBase::default_energy_function func(1.0);

            if (method == "dag") {
                cerr << "Method: DAG" << "\n";
                auto packer = makeSaPacker<
                    DagPackGenerator<boost::fast_pool_allocator<char>>>(opts, func);
                run_packer(packer, layout, begin(nets), end(nets), *out, verbose_level);
            } else if (method == "lcs") {
                cerr << "Method: LCS" << "\n";
                auto packer = makeSaPacker<
                    LcsPackGenerator<boost::fast_pool_allocator<char>>>(opts, func);
                run_packer(packer, layout, begin(nets), end(nets), *out, verbose_level);
            } else {
                assert(false);
            }
        }

    } catch (const std::exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
