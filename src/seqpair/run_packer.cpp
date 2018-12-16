// run_packer.cpp: entry to sequence pair project.
// Author: LYL (Aureliano Lee)

#include "xseqpair.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/pool/pool_alloc.hpp>
#include "timeit.h"
#include "toolbox.h"
#include "layout.h"
#include "pack_generator.h"
#include "sa_packer.h"
#include "verification.h"

using namespace std;
using namespace seqpair;

namespace {

    template<typename Generator, typename Alloc, typename FwdIt>
    void run_packer(SaPacker<Generator> &packer, Layout<Alloc> &layout, 
        FwdIt first_line, FwdIt last_line, ostream &out, 
        unsigned verbose_level) {
        using namespace seqpair::verification;
        using change_t = PackGeneratorBase::change_t;

        cout << packer.options();

        // Change distribution and runtime allocator
        // Note: maybe pool_options can be specified
        PackGeneratorBase::default_change_distribution chg_dist;

        double cost = 0;
        auto runtime = aureliano::timeit([&] {
            cost = packer(layout, first_line, last_line,
                chg_dist, verbose_level);
        });

        cout << "\n";
        cout << "Runtime: " <<
            chrono::duration_cast<chrono::milliseconds>(runtime).count() <<
            "ms" << "\n";
        auto sum_rect_areas = layout.sum_conponent_areas();
        cout << "Sum of rectangle areas: " << sum_rect_areas << "\n";
        auto sln_area = layout.get_area();
        {
            using namespace seqpair::io;
            cout << "Area: " << sln_area.first * sln_area.second <<
                " " << sln_area << "\n";
        }
        cout << "Utilization: " << 1.0 * sum_rect_areas /
            (sln_area.first * sln_area.second) << "\n";
        auto wirelen = sum_manhattan_distances(layout, first_line, last_line);
        cout << "Wirelength: " << wirelen << "\n";
        cout << "Cost: " << cost << "\n";

        auto alpha = packer.energy_function().alpha;
        if (abs((alpha * sln_area.first * sln_area.second + (1 - alpha) * wirelen) / cost - 1) > 1e-5)
            cout << "Wrong answer: incorrect cost." << "\n";
        else if (has_intersection(layout))
            cout << "Wrong answer: layout contains intersections." << "\n";
        else
            cout << "Answer accepted.\n";

        using format_policy = typename Layout<Alloc>::format_policy;
        out << layout.format(format_policy::no_delim);
    }

    void print_usage() {
        cout << "Usage: rect_file, net_file, alpha, method, "
            "result_file [verbose_level=1] [option_file]" << "\n";
    }
}


int main(int argc, char **argv) {
    try {
        bool is_argv_valid = false;
        if (argc < 6) {
            print_usage();
            return EXIT_FAILURE;
        }

        string rect_file = argv[1], net_file = argv[2];
        double alpha = strtod(argv[3], nullptr);
        string method = argv[4];
        string result_file = argv[5];
        unsigned verbose_level = 1;
        if (argc > 6)
            verbose_level = strtoul(argv[6], nullptr, 10);
        string opt_file;
        if (argc > 7)
            opt_file = argv[7];
        if (argc > 8)
            cout << "Warning: extra command-line arguments are ommitted." << "\n";

        for (auto &e : method)
            e = tolower(e);
        if (method == "lcs" || method == "dag")
            is_argv_valid = true;
        if (!is_argv_valid) {
            print_usage();
            return EXIT_FAILURE;
        }

        Layout<> layout;
        {
            ifstream in(rect_file);
            if (!in.is_open())
                throw runtime_error("Cannot open file");
            in >> layout;
        }

        vector<pair<size_t, size_t>> nets;
        {
            ifstream in(net_file);
            if (!in.is_open())
                throw runtime_error("Cannot open file");
            size_t i, j;
            while (in >> i >> j) {
                nets.emplace_back(i, j);
            }
            using const_reference = typename decltype(nets)::const_reference;
            if (any_of(nets.cbegin(), nets.cend(), [&](const_reference p) {
                return p.first >= layout.size() || p.second >= layout.size(); }))
                throw invalid_argument("Net index out of range");
        }

        SaPackerBase::options_t opts;
        if (!opt_file.empty()) {
            ifstream in(opt_file);
            if (!in.is_open())
                throw runtime_error("Cannot open file");
            in >> opts; // Params
        } else {
            opts.simulaions_per_temperature = max(30 * layout.size(), static_cast<size_t>(1024));
        }

        cout << "Rectangles: " << layout.size() << "\n";
        cout << "Alpha: " << alpha << "\n" << "\n";
        SaPackerBase::default_energy_function func(alpha);
        {
            ofstream out(result_file);
            if (method == "dag") {
                cout << "Method: DAG" << "\n";
                auto packer = makeSaPacker<DagPackGenerator<boost::fast_pool_allocator<char>>>(opts, func);
                run_packer(packer, layout, begin(nets), end(nets), out, verbose_level);

            } else if (method == "lcs") {
                cout << "Method: LCS" << "\n";
                auto packer = makeSaPacker<LcsPackGenerator<boost::fast_pool_allocator<char>>>(opts, func);
                run_packer(packer, layout, begin(nets), end(nets), out, verbose_level);
                
            } else {
                assert(false);
            }
        }

    } catch (std::exception &e) {
        cout << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}