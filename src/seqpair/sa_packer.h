// sa_packer.h: SaPacker (optimizer) using SA.
// Author: LYL (Aureliano Lee)

#pragma once
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
#include <boost/pool/pool_alloc.hpp>
#include "layout.h"
#include "pack_generator.h"

namespace seqpair {
    // Wirelength.
    template<typename Alloc, typename FwdIt>
    double sum_manhattan_distances(const Layout<Alloc> &layout,
        FwdIt first, FwdIt last) {
        using namespace std;
        int64_t twice = 0;
        for (auto i = first; i != last; ++i) {
            auto c0 = (layout.x()[get<0>(*i)] << 1) + layout.widths()[get<0>(*i)];
            auto c1 = (layout.x()[get<1>(*i)] << 1) + layout.widths()[get<1>(*i)];
            twice += abs(c1 - c0);
        }
        for (auto i = first; i != last; ++i) {
            auto c0 = (layout.y()[get<0>(*i)] << 1) + layout.heights()[get<0>(*i)];
            auto c1 = (layout.y()[get<1>(*i)] << 1) + layout.heights()[get<1>(*i)];
            twice += abs(c1 - c0);
        }
        return twice / 2.0;
    }

    // Default packing cost.
    template<typename Alloc, typename FwdIt>
    double packing_cost(const Layout<Alloc> &layout, FwdIt first, FwdIt last,
        int w, int h, double alpha) {
        auto area = w * h;
        auto len = sum_manhattan_distances(layout, first, last);
        return alpha * area + (1 - alpha) * len;
    }

    // Base of SaPacker with default types.
    struct SaPackerBase {
        // Evaluation function (packing_cost with binded alpha). 
        struct default_energy_function {
            default_energy_function() :
                default_energy_function(1.0) { }
            default_energy_function(double alpha) : alpha(alpha) {}

            template<typename Alloc, typename FwdIt>
            double operator()(const Layout<Alloc> &layout, FwdIt first,
                FwdIt last, int w, int h) const {
                return packing_cost(layout, first, last, w, h, alpha);
            }

            double alpha;
        };

        // Options for simulated annealing.
        struct options_t {
            options_t() = default;
            options_t(double init_accept_prob, std::size_t reps_per_t, 
                double dec_ratio, double restart_r, double stop_accept_prob) :
                initial_accepting_probability(init_accept_prob),
                decreasing_ratio(dec_ratio),
                simulaions_per_temperature(reps_per_t),
                restart_ratio(restart_r),
                stopping_accepting_probability(stop_accept_prob) { }

            double initial_accepting_probability = 0.9;
            std::size_t simulaions_per_temperature = 1024;
            double decreasing_ratio = 0.9;
            double restart_ratio = 2;
            double stopping_accepting_probability = 0.05;
        };
    };

    std::istream &operator>>(std::istream &in, typename SaPackerBase::options_t &opts) {
        in >> opts.initial_accepting_probability;
        in >> opts.simulaions_per_temperature;
        in >> opts.decreasing_ratio;
        in >> opts.restart_ratio;
        in >> opts.stopping_accepting_probability;
        return in;
    }

    std::ostream &operator<<(std::ostream &out, const typename SaPackerBase::options_t &opts) {
        using namespace std;
        cerr << "initial_accepting_probability: " << opts.initial_accepting_probability << "\n";
        cerr << "simulations_per_temperature: " << opts.simulaions_per_temperature << "\n";
        cerr << "decreasing_ratio: " << opts.decreasing_ratio << "\n";
        cerr << "restart_ratio: " << opts.restart_ratio << "\n";
        cerr << "stopping_accepting_probability: " << opts.stopping_accepting_probability << "\n";
        return out;
    }

    // Simulated annealing packer.
    template<typename Generator, typename EFunc = 
        typename SaPackerBase::default_energy_function>
    class SaPacker : public SaPackerBase {
        using self_t = SaPacker;
        using base_t = SaPackerBase;

    public:
        using typename base_t::options_t;
        using typename base_t::default_energy_function;
        using generator_t = typename Generator::unbuffered_generator_t;
        using energy_function_t = EFunc;
        
    protected:
        using generator_allocator_type = typename generator_t::allocator_type;
        using generator_default_change_distribution = 
            typename generator_t::default_change_distribution;

    public:
        explicit SaPacker(const options_t &opts = options_t(),
            const energy_function_t &func = energy_function_t(),
            const generator_allocator_type &alloc = generator_allocator_type()) : 
            opts_(check_option(opts)), energy_func_(func), eng_(SEQPAIR_RANDOM_SEED()), 
            generator_(alloc) { }

        const options_t &options() const {
            return opts_;
        }

        void set_options(const options_t &opts) {
            opts_ = check_option(opts);
        }

        const energy_function_t &energy_function() const {
            return energy_func_;
        }

        void set_energy_function(const energy_function_t &func) {
            energy_func_ = func;
        }

        const generator_t &generator() const {
            return generator_;
        }

        // Generates the solution and writes it to layout.
        template<typename LayoutAlloc, typename FwdIt,
            typename ChgDist = generator_default_change_distribution>
            double operator()(Layout<LayoutAlloc> &layout, FwdIt first_line, FwdIt last_line,
                ChgDist &&chg_dist = ChgDist(), //Alloc &&alloc = Alloc(), 
                int verbose_level = 1) {
            using namespace std;
            if (layout.empty())
                return 0;

            size_t num_simulations = 0;

            // Deferred generator construction from layout.
            generator_.construct(layout.widths(), layout.heights(), eng_); 
            auto best_gen = generator_;
            auto res = generator_.make_resource();
            
            // Initial loop for determining starting temperature.
            auto local_layout = layout;
            auto best_layout = layout;
            double min_energy = numeric_limits<double>().max(), 
                max_energy = numeric_limits<double>().min();
            double curr_energy, last_energy;
            double sum_energies = 0, sum_sqrs = 0;   // For stddev
            
            if (verbose_level) cerr << "\n";
            constexpr size_t init_sims = 64;  
            for (size_t i = 0; i != init_sims; ++i) {
                int w, h;
                std::tie(w, h) = generator_(local_layout, eng_, res, chg_dist);
                curr_energy = energy_func_(local_layout, first_line, last_line, w, h);
                ++num_simulations;
                if (curr_energy < min_energy) {
                    detail::unguarded_copy_layout(local_layout, best_layout);
                    detail::unguarded_copy_generator(generator_, best_gen);
                    min_energy = curr_energy;
                }
                sum_energies += curr_energy;
                sum_sqrs += curr_energy * curr_energy;
                max_energy = max(max_energy, curr_energy);
                last_energy = curr_energy;
                generator_.shuffle(eng_);
            }
            
            auto stddev = sqrt((sum_sqrs - sum_energies * sum_energies / init_sims) / 
                (init_sims - 1));
            double temp = (stddev + numeric_limits<double>().epsilon()) / 
                log(1.0 / opts_.initial_accepting_probability);

            if (verbose_level) {
                cerr << "Starting temperature: " << temp << "\n";
                cerr << "Starting min energy: " << min_energy << "\n";
                cerr << "Starting max energy: " << max_energy << "\n";
                cerr << "Stddev: " << stddev << "\n";
                if (verbose_level >= 2)
                    cerr << "\n";
            }

            // Main simulation process.
            constexpr double temp_guard = 1.0;
            uniform_real_distribution<> rand_double(0, 1);
            size_t num_restarts = 0;

            for (;;) {
                size_t num_acceptions = 0;
                double my_sum_energies = 0;

                for (size_t i = 0; i != opts_.simulaions_per_temperature; ++i) {
                    int w, h;
                    std::tie(w, h) = generator_(local_layout, eng_, res, chg_dist);
                    ++num_simulations;
                    auto new_energy = energy_func_(local_layout, first_line, last_line, w, h);
                    my_sum_energies += new_energy;

                    if (new_energy < curr_energy ||
                        rand_double(eng_) < exp((curr_energy - new_energy) / temp)) {
                        if (new_energy < min_energy) {
                            detail::unguarded_copy_layout(local_layout, best_layout);
                            detail::unguarded_copy_generator(generator_, best_gen);
                            min_energy = new_energy;
                        }
                        curr_energy = new_energy;
                        ++num_acceptions;
                    } else {
                        check_undo(std::forward<ChgDist>(chg_dist));
                    }
                }
                
                if (verbose_level >= 2) {
                    cerr << "Temperature: " << temp << ", average energy: " <<
                        my_sum_energies / opts_.simulaions_per_temperature <<
                        ", acception rate: " << static_cast<double>(num_acceptions) /
                        opts_.simulaions_per_temperature << "\n";
                }

                // Terminate criterion
                if (static_cast<double>(num_acceptions) < 
                    opts_.stopping_accepting_probability * opts_.simulaions_per_temperature ||
                    temp < temp_guard)  // Usually this doens't happen, in certain cases this is necessary
                    break;

                // Restart if necessary
                // Note: based on average or current? (experiment shows that average-based 
                // restart is better)
                if (my_sum_energies / opts_.simulaions_per_temperature >
                    opts_.restart_ratio * min_energy) {
                    detail::unguarded_copy_layout(best_layout, local_layout);  // Not compulsory
                    detail::unguarded_copy_generator(best_gen, generator_);
                    curr_energy = min_energy;
                    ++num_restarts;
                }

                // Drop temperature
                temp *= opts_.decreasing_ratio;
            }

            // Output results
            if (verbose_level) {
                cerr << "\n";
                cerr << "Finishing temperature: " << temp << "\n";
                cerr << "Finishing energy: " << curr_energy << "\n";
                cerr << "Total simulations: " << num_simulations << "\n";
                cerr << "Total restarts: " << num_restarts << "\n";
            }
            layout = std::move(best_layout);
            return min_energy;
        }

    protected: 
        // Checks option.
        bool is_option_valid(const options_t &opts) const noexcept {
            return opts.initial_accepting_probability > 0 &&
                opts.initial_accepting_probability < 1 &&
                opts.simulaions_per_temperature &&
                opts.decreasing_ratio > 0 &&
                opts.decreasing_ratio < 1 &&
                opts.restart_ratio > 1 &&
                opts.stopping_accepting_probability > 0 &&
                opts.stopping_accepting_probability <= 1;
        }

        // Throws on invalid option.
        const options_t &check_option(const options_t &opts) const {
            if (!is_option_valid(opts))
                throw std::invalid_argument("Invalid argument");
            return opts;
        }

        // Invokes generator_t::rollback and checks the return value.
        template<typename ChgDist>
        bool check_undo(ChgDist &&chg_dist) {
            return check_undo(generator_, chg_dist);
        }

        // Invokes generator_t::rollback and checks the return value.
        template<typename ChgDist>
        bool check_undo(generator_t &gen, ChgDist &&chg_dist) const {
            auto b = gen.rollback();
            assert(detail::may_change_be_none(std::forward<ChgDist>(chg_dist)) || b);
            return b;
        }

        // Note: actually energy_func_ had better be stored in boost::compressed_pair
        options_t opts_;
        energy_function_t energy_func_; 
        std::default_random_engine eng_;
        generator_t generator_;
    };

    // Helper function for constructing SaPacker.
    template<typename Generator, typename EFunc =
        typename SaPackerBase::default_energy_function, typename... Types>
        SaPacker<Generator, std::decay_t<EFunc>>
        makeSaPacker(const typename SaPackerBase::options_t &opts =
            typename SaPackerBase::options_t(), EFunc &&func = EFunc(),
            Types &&...args) {
        return SaPacker<Generator, std::decay_t<EFunc>>(opts,
            std::forward<EFunc>(func), std::forward<Types>(args)...);
    }
}
