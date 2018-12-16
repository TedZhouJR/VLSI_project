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

    void print_usage() {
        cerr << "Usage: rect_file\n"
            "[result_file=cout] [method=lcs] [verbose_level=1] [option_file]\n";
    }
}

int main(int argc, char **argv) {
    try {
        bool is_argv_valid = false;
        if (argc < 2) {
            print_usage();
            return EXIT_FAILURE;
        }

        string rect_file = argv[1], result_file = "cout";
        if (argc > 2)
            result_file = argv[2];
        double alpha = 1;
        string method = "lcs";
        if (argc > 3)
            method = argv[3];
        unsigned verbose_level = 1;
        if (argc > 4)
            verbose_level = strtoul(argv[4], nullptr, 10);
        string opt_file;
        if (argc > 5)
            opt_file = argv[5];
        if (argc > 6)
            cerr << "Warning: extra command-line arguments are ommitted." << "\n";

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

        SaPackerBase::options_t opts;
        if (!opt_file.empty()) {
            ifstream in(opt_file);
            if (!in.is_open())
                throw runtime_error("Cannot open file");
            in >> opts; // Params
        } else {
            opts.simulaions_per_temperature = max(30 * layout.size(), static_cast<size_t>(1024));
        }

        cerr << "Rectangles: " << layout.size() << "\n";
        cerr << "Alpha: " << alpha << "\n" << "\n";
        SaPackerBase::default_energy_function func(alpha);
        
        ostream *out = &cout;
        ofstream fout;
        if (result_file != "cout") {
            fout.open(result_file);
            out = &fout;
        }

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

    } catch (std::exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}