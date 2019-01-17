//  main.cpp
//  simulate anneal

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include <boost/interprocess/sync/null_mutex.hpp>
#include <boost/program_options.hpp>
#include <boost/pool/pool_alloc.hpp>

#include "timeit.h"
#include "toolbox.h"
#include "layout.h"
#include "pack_generator.h"
#include "sa_packer.h"
#include "verify.hpp"
#include "verification.h"
#include "interpreter.h"
#include "sa.hpp"

using namespace std;
using namespace seqpair;
namespace po = boost::program_options;

using combine_type = polish::meta_polish_node::combine_type;
using dimension_type = polish::meta_polish_node::dimension_type;
using vtree_type = polish::vectorized_polish_tree<
    boost::fast_pool_allocator<
        polish::basic_vectorized_polish_node<
            boost::fast_pool_allocator<polish::meta_polish_node::coord_type, 
            boost::default_user_allocator_new_delete,
            boost::interprocess::null_mutex>
        >, 
        boost::default_user_allocator_new_delete,
        boost::interprocess::null_mutex
    >
>;
using tree_type = polish::polish_tree<
    boost::fast_pool_allocator<
        polish::basic_polish_node, 
        boost::default_user_allocator_new_delete,
        boost::interprocess::null_mutex
    >
>;
using char_allocator = boost::fast_pool_allocator<
    char,
    boost::default_user_allocator_new_delete,
    boost::interprocess::null_mutex
>;

namespace {

    template<typename Generator, typename Alloc, typename FwdIt>
    void run_packer(SaPacker<Generator> &packer, Layout<Alloc> &layout,
        FwdIt first_line, FwdIt last_line, ostream &out,
        int verbose_level) {
        using namespace seqpair::verification;
        using change_t = PackGeneratorBase::change_t;

        cerr << packer.options();

        // Change distribution 
        PackGeneratorBase::default_change_distribution chg_dist;

        double cost = 0;
        auto runtime = aureliano::timeit([&] {
            cost = packer(layout, first_line, last_line,
                chg_dist, verbose_level);
        });

        cerr << "\n";
        cerr << "Runtime: " << static_cast<double>(
            chrono::duration_cast<chrono::milliseconds>(runtime).count()) / 1000 <<
            "s" << "\n";
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
        if (abs((alpha * sln_area.first * sln_area.second 
            + (1 - alpha) * wirelen) / cost - 1) > 1e-5)
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

    void run_vectorized_polish_tree(const yal::Interpreter &interpreter, 
        int rounds, std::ostream &out) {
        using namespace polish;
        cerr <<  "Start simulate annealing..." << endl;
        vtree_type vtree;
        auto module_index = interpreter.make_module_index();
        default_random_engine eng(random_device{}()); 
        vtree.construct(interpreter.modules().cbegin(),
            module_index.cbegin(), module_index.cend(), eng);

        double init_accept_rate = 0.95, cooldown_speed = 0.01, ending_temperature = 20;
        std::int64_t utility_stable = 0, pre_utility = 0, utility = 0;
        while (utility_stable < rounds) {
            SA<vtree_type> sa(vtree, init_accept_rate,
                cooldown_speed, ending_temperature, eng);
            while (!sa.reach_end()) {
                while (!sa.reach_balance()) {
                    sa.take_step(eng);
                }
                sa.cool_down_by_ratio();
            }
            sa.print_statistics();
            utility = sa.get_best_area();
            if (pre_utility == utility) {
                utility_stable++;
            } else {
                utility_stable = 0;
            }
            vtree = sa.get_best_tree();
            pre_utility = utility;
        }
        
        std::vector<typename vtree_type::floorplan_entry> result;
        std::size_t best_point = SA<vtree_type>::get_best_point(vtree);
        vtree.floorplan(best_point, back_inserter(result));
        for (auto &&e: result) {
            out << std::get<0>(e) << " " << std::get<1>(e) 
                << " " << std::get<2>(e) << " " << std::get<3>(e) << std::endl;
        }
        if (polish::overlap(result.begin(), result.end()))
            std::cerr << "Overlap error!!" << std::endl;
        else
            std::cerr << "Answer accepted." << std::endl;
    }

    void run_polish_tree(const yal::Interpreter &interpreter,
        int rounds, std::ostream &out) {
        using namespace polish;
        cerr << "Start simulate annealing..." << endl;
        tree_type tree;
        auto module_index = interpreter.make_module_index();
        default_random_engine eng(random_device{}());
        tree.construct(interpreter.modules().cbegin(),
            module_index.cbegin(), module_index.cend(), eng);

        double init_accept_rate = 0.95, cooldown_speed = 0.01, ending_temperature = 20;
        std::int64_t utility_stable = 0, pre_utility = 0, utility = 0;
        while (utility_stable < rounds) {
            SA<tree_type> sa(tree, init_accept_rate,
                cooldown_speed, ending_temperature, eng);
            while (!sa.reach_end()) {
                while (!sa.reach_balance()) {
                    sa.take_step(eng);
                }
                sa.cool_down_by_ratio();
            }
            sa.print_statistics();
            utility = sa.get_best_area();
            if (pre_utility == utility) {
                utility_stable++;
            } else {
                utility_stable = 0;
            }
            tree = sa.get_best_tree();
            pre_utility = utility;
        }

        std::vector<typename tree_type::floorplan_entry> result;
        tree.floorplan(back_inserter(result));
        std::vector<std::tuple<dimension_type, dimension_type,
            dimension_type, dimension_type>> detailed_result;
        detailed_result.reserve(result.size());
        auto it = tree.begin();
        for (auto &&e : result) {
            detailed_result.emplace_back(std::get<0>(e), 
                std::get<1>(e), it->width, it->height);
            out << std::get<0>(e) << " " << std::get<1>(e) 
                << " " <<  it->width << " " << it->height << std::endl;
            do {
                ++it;
            } while (it != tree.end() && it->type != combine_type::LEAF);
        }
        if (polish::overlap(detailed_result.cbegin(), detailed_result.cend())) 
            std::cerr << "Overlap error!!" << std::endl;
        else
            std::cerr << "Answer accepted." << std::endl;
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
        ("rounds,r", po::value<int>()->default_value(10),
            "required stable rounds for polish-curve/polish to stop")
        ("option,O", po::value< vector<string> >(),
            "option file for lcs/dag")
        ("method,m", po::value< vector<string> >(),
            "method (polish-curve/polish/lcs/dag, default polish-curve)")
        ("verbose,v", po::value<int>()->default_value(1)->implicit_value(2),
            "verbose level (0-2)")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cerr <<  desc << "\n";
        return EXIT_SUCCESS;
    }

    try {
        string method = "polish-curve";
        if (vm.count("method")) {
            method = vm["method"].as<vector<string>>().back();
            for (auto &e : method)
                e = tolower(e);
            if (method != "lcs" && method != "dag" 
                && method != "polish" && method != "polish-curve")
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
        if (interpreter.parent_module().network.empty())
            throw runtime_error("Modules empty!");

        ostream *out = &cout;
        ofstream fout;
        if (vm.count("output")) {
            fout.open(vm["output"].as<vector<string>>().back());
            out = &fout;
        }

        if (method == "polish" || method == "polish-curve") {
            cerr <<  "Method: " << method << endl;
            int rounds = vm["rounds"].as<int>();
            if (rounds <= 0) {
                cerr << "Warning: non-positive rounds argument; using 10." << endl;
                rounds = 10;
            }
            cerr << "Stable rounds: " << rounds << endl;

            auto runtime = method == "polish" ?
                aureliano::timeit([&] { 
                    run_polish_tree(interpreter, rounds, *out); 
                }) :
                aureliano::timeit([&] { 
                    run_vectorized_polish_tree(interpreter, rounds, *out); 
                });

            cerr << "Runtime: " << static_cast<double>(
                    chrono::duration_cast<chrono::milliseconds>(runtime).count()) / 1000 <<
                "s" << "\n";

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

            int verbose_level = vm["verbose"].as<int>();

            vector<pair<size_t, size_t>> nets;

            cerr << "Rectangles: " << layout.size() << "\n";
            SaPackerBase::default_energy_function func(1.0);

            if (method == "dag") {
                cerr << "Method: DAG" << "\n";
                auto packer = makeSaPacker<DagPackGenerator<char_allocator>>(opts, func);
                run_packer(packer, layout, begin(nets), end(nets), *out, verbose_level);
            } else if (method == "lcs") {
                cerr << "Method: LCS" << "\n";
                auto packer = makeSaPacker<LcsPackGenerator<char_allocator>>(opts, func);
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
