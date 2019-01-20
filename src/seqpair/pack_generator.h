// pack_generator.h: class DagPackGenerator, class LcsPackGenerator 
//      and helper functions.
// Rewritten by LYL (Aureliano Lee)

#pragma once
#include "xseqpair.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <tuple>
#include <utility>
#include <vector>
#include <boost/container/pmr/map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dag_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include "toolbox.h"
#include "layout.h"

namespace seqpair {
    namespace detail {
        // Makes left inversion of range [first, last) that is permutation 
        // of [0, n).
        template<typename InIt, typename RanIt>
        auto make_left_inverse(InIt first, InIt last, RanIt rev) {
            using value_type = typename std::iterator_traits<InIt>::value_type;
            value_type i = 0;
            for (; first != last; ++first)
                rev[*first] = i++;
            return i;
        }

        // Let match = inv(y) * x, where x, y are permutations of [0, n).
        // Assuming buffer is big enough to hold [0, n).
        template<typename InIt0, typename InIt1, typename OutIt, typename RanIt>
            OutIt make_match(InIt0 y_begin, InIt0 y_end, InIt1 x_begin,
                OutIt match, RanIt buffer) {
            using value_type = typename std::iterator_traits<InIt0>::value_type;
            // Let buffer[y[i]] = i
            auto sz = make_left_inverse(y_begin, y_end, buffer);
            // Let match = inv(y) * x, s.t. x == y * match
            for (value_type j = 0; j != sz; ++j)
                *match++ = buffer[*x_begin++];
            return match;
        }

        // Fast LCS evaluation in O(nlogn).
        template<typename FwdIt0, typename FwdIt1,
            typename RanIt0, typename RanIt1,
            typename RanIt2, typename RanIt3, typename Map>
            auto eval_sp2(FwdIt0 y_begin, FwdIt0 y_end,         // in
                FwdIt1 x_begin, RanIt0 len,                     // in
                RanIt1 pos,                                     // out
                RanIt2 buffer, RanIt3 match, Map &&pq) {        // auxilary
            static_assert(std::is_signed<typename std::decay_t<Map>::key_type>::value,
                "Map must support signed keys");

            seqpair::detail::make_match(y_begin, y_end, x_begin, match, buffer);

            pq.clear();
            pq.emplace(-1, 0);
            auto sz = std::distance(y_begin, y_end);
            for (decltype(sz) i = 0; i != sz; ++i) {
                auto b = *x_begin++;
                auto p = match[i];
                auto it = pq.emplace(p, 0).first;   // .second is bool
                pos[b] = std::prev(it)->second;
                auto t = pos[b] + len[b];
                it->second = t;
                ++it;
                while (it != pq.end()) {
                    if (it->second <= t) {
                        it = pq.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            assert(!pq.empty());
            return pq.rbegin()->second;
        }

        // Empty tags to identify whether I'm buffered.
        struct UnbufferedGeneratorTag { };
        struct BufferedGeneratorTag { };

        // Base class to define enum change_t and functor default_change_distribution
        struct PackGeneratorBase {
            // Enum of next move.
            enum class change_t {
                none, rotate,
                swap_x, swap_y, swap_xy,
                reverse_x, reverse_y, reverse_xy,
                rotate_x, rotate_y, rotate_xy
            };

            static constexpr size_t change_t_size =
                static_cast<size_t>(change_t::rotate_xy) + 1;

            // Functor for deciding next move. Meets the concept of 
            // ChangeDistribution. Deterministic or stateful ChangeDistribution
            // can also be used for sequence pair evaluation.
            class default_change_distribution {
            protected:
                using change_t = typename PackGeneratorBase::change_t;
                using array_t = std::array<double, change_t_size>;

            public:
                using result_type = change_t;

                // Default constructs
                default_change_distribution() {
                    using std::make_pair;
                    assign_from_map({
                        make_pair(change_t::reverse_x, 1.0),
                        make_pair(change_t::reverse_y, 1.0),
                        make_pair(change_t::reverse_xy, 1.0),
                        make_pair(change_t::rotate_x, 1.0),
                        make_pair(change_t::rotate_y, 1.0),
                        make_pair(change_t::rotate_xy, 1.0),
                        make_pair(change_t::rotate, 6.0 / 9)
                        });
                }

                // Constructs from probabilities of each option.
                template<typename InIt>
                default_change_distribution(InIt first, InIt last) {
                    assign(first, last);
                }

                // Constructs from change_t, probability pairs.
                template<typename InIt>
                static default_change_distribution from_map(InIt first, InIt last) {
                    default_change_distribution dist;
                    dist.assign_from_map(first, last);
                    return dist;
                }

                // Constructs from change_t, probability pairs.
                template<typename Pair>
                static default_change_distribution from_map(std::initializer_list<Pair> ilist) {
                    return from_map(ilist.begin(), ilist.end());
                }

                // Assigns from probabilities of each change.
                // Throws: invalid_argument if any value is negative 
                //      (strong exception guarantee).
                template<typename InIt>
                void assign(InIt first, InIt last) {
                    using namespace std;
                    array_t temp;
                    copy(first, last, temp.begin());
#ifdef SEQPAIR_PACK_GENERATOR_OMIT_NONE_CHANGE
                    temp[static_cast<size_t>(change_t::none)] = 0;
#endif
                    if (!all_of(temp.cbegin(), temp.cend(), [](auto x) { return x >= 0.0; }))
                        throw invalid_argument("Negative probability");
                    auto eps = numeric_limits<double>().epsilon();
                    for (auto &e : temp)
                        e += eps;
                    partial_sum(temp.begin(), temp.end(), temp.begin());
                    auto sum = temp.back();
                    for (auto &e : temp)
                        e /= sum;
                    assert(temp.back() == 1.0);
                    df_ = temp;
                }

                // Assigns from change_t, probabiilty pairs.
                // Throws: invalid_argument if any probability is negative 
                //      (strong exception guarantee).
                template<typename InIt>
                void assign_from_map(InIt first, InIt last) {
                    using namespace std;
                    array_t probs;
                    probs.fill(0.0);
                    change_t chg; double p;
                    for (; first != last; ++first) {
                        tie(chg, p) = *first;
                        probs[static_cast<size_t>(chg)] += p;
                    }
                    if (any_of(probs.cbegin(), probs.cend(), [](auto x) { return x < 0.0; }))
                        throw invalid_argument("Negative probability");
                    assign(probs.cbegin(), probs.cend());
                }

                // Assigns from change_t, probabiilty pairs.
                template<typename Pair>
                void assign_from_map(std::initializer_list<Pair> ilist) {
                    assign_from_map(ilist.begin(), ilist.end());
                }

                template<typename Eng>
                result_type operator()(Eng &&eng) const {
                    auto x = std::uniform_real_distribution<>(0, 1)(std::forward<Eng>(eng));
                    auto k = std::lower_bound(df_.cbegin(), df_.cend(), x) - df_.cbegin();
                    assert(k != df_.size());
                    if (k == df_.size())    // Shouldn't happen
                        --k;
                    return static_cast<result_type>(k);
                }

                bool maybe_none() const {
                    return df_[static_cast<size_t>(change_t::none)] == 0;
                }

            protected:
                array_t df_;  // Distribution function
            };

            // Factory of default_change_distribution.
            template<typename... Types>
            static default_change_distribution 
                make_default_change_distribution(Types &&...args) {
                return default_change_distribution(std::forward<Types>(args)...);
            }
        };

        // Traits for ChangeDistribution concept.
        template<typename ChgDist>
        struct IsChangeNeverNone : public std::false_type { };

#ifdef SEQPAIR_PACK_GENERATOR_OMIT_NONE_CHANGE
        // Traits for ChangeDistribution concept.
        template<>
        struct IsChangeNeverNone<typename PackGeneratorBase::default_change_distribution> : 
            public std::true_type { };
#endif

        template<typename ChgDist>
        bool may_change_be_none(ChgDist &&chg_dist) {
            return may_change_be_none_impl(std::forward<ChgDist>(chg_dist), 
                IsChangeNeverNone<std::decay_t<ChgDist>>());
        }

        template<typename ChgDist>
        bool may_change_be_none_impl(ChgDist &&chg, std::true_type) {
            return false;
        }

        template<typename ChgDist>
        bool may_change_be_none_impl(ChgDist &&chg, std::false_type) {
            return std::forward<ChgDist>(chg).maybe_none();
        }

        // Graph-based sequence-pair packing generator which does not own buffer resource.
        // Note: modified allocator<void> to allocator<void *>, for compilation with g++.
        template<typename Alloc = std::allocator<void *>>
        class DagPackGeneratorBase : public PackGeneratorBase {
            using self_t = DagPackGeneratorBase<Alloc>;
            using base_t = PackGeneratorBase;

        protected:
            using int_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<int>;
            using size_t_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<size_t>;
            using size_vector_t = std::vector<int, int_alloc_t>;
            using sequence_pair_t = std::vector<std::size_t, size_t_alloc_t>;
            using momento_t = std::tuple<change_t, std::size_t, std::size_t>;

        public:
            using typename base_t::change_t;
            using typename base_t::default_change_distribution;
            using allocator_type = Alloc;
            using resource_t = std::vector<char, allocator_type>;
            using generator_tag = UnbufferedGeneratorTag;
            
            DagPackGeneratorBase() : DagPackGeneratorBase(allocator_type()) { }

            DagPackGeneratorBase(const self_t &) = default;
            DagPackGeneratorBase(self_t &&) = default;

            explicit DagPackGeneratorBase(const allocator_type &alloc) : 
                widths_(alloc), heights_(alloc), sp_x_(alloc), sp_y_(alloc),
                last_change_(change_t::none, 0, 0) { }

            template<typename Cont0, typename Cont1, typename Eng>
                DagPackGeneratorBase(Cont0 &&widths, Cont1 &&heights,
                    Eng &&eng, const allocator_type &alloc = allocator_type()) :
                widths_(std::begin(std::forward<Cont0>(widths)), 
                    std::end(std::forward<Cont0>(widths)), alloc),
                heights_(std::begin(std::forward<Cont1>(heights)),
                    std::end(std::forward<Cont1>(heights)), alloc),
                sp_x_(this->size(), alloc), sp_y_(this->size(), alloc),
                last_change_(change_t::none, 0, 0) {
                assert(widths_.size() == heights_.size());
                std::iota(sp_x_.begin(), sp_x_.end(), 0);
                std::iota(sp_y_.begin(), sp_y_.end(), 0);
                this->shuffle(std::forward<Eng>(eng));
            }

            self_t &operator=(const self_t &) = default;
            self_t &operator=(self_t &&) = default;

            // Makes a resource object that can be shared.  
            resource_t make_resource() const {
                // Allocator cannot be easily obtained from members :(
                return resource_t(min_buffer_size(), widths_.get_allocator());
            }

            // Constructs from given args. This invalidates the subsequent call
            // to rollback. Multiple constructs are allowed.
            template<typename Cont0, typename Cont1, typename Eng>
            void construct(Cont0 &&widths, Cont1 &&heights, Eng &&eng) {
                using namespace std;
                widths_.assign(begin(std::forward<Cont0>(widths)), 
                    end(std::forward<Cont0>(widths)));
                heights_.assign(begin(std::forward<Cont1>(heights)), 
                    end(std::forward<Cont1>(heights)));
                assert(widths_.size() == heights_.size());
                auto sz = widths_.size();
                sp_x_.resize(sz);
                sp_y_.resize(sz);
                iota(sp_x_.begin(), sp_x_.end(), 0);
                iota(sp_y_.begin(), sp_y_.end(), 0);
                this->shuffle(std::forward<Eng>(eng));
                last_change_ = forward_as_tuple(change_t::none, 0, 0);
            }

            // Computes packing layout, writes result to layout, and changes
            // next internal state.
            // Returns: (width, height)
            template<typename LayoutAlloc, typename Eng,
                typename ChgDist = default_change_distribution>
                std::pair<int, int> operator()(Layout<LayoutAlloc> &layout,
                    Eng &&eng, resource_t &res, ChgDist &&chg_dist = ChgDist()) {
                assert(layout.size() == this->size());
                // Change to next state, widths and heights may change
                change(std::forward<Eng>(eng), std::forward<ChgDist>(chg_dist));
                // Synchronize widths and heights
                unguarded_copy_layout_sizes(layout);
                // Evaluate current state
                return eval(layout, std::forward<Eng>(eng), res);
            }

            // One-shot rollback. If cannot rollback, does nothing.
            // Cannot restore changed Layout.
            bool rollback() {
                using namespace std;
                change_t chg; size_t i, j; bool ans = true;
                std::tie(chg, i, j) = last_change_;

                switch (chg) {
                case change_t::none:
                    assert(false);
                    ans = false;
                    break;

                case change_t::rotate:
                    assert(i == j);
                    unrotate_component(i);
                    break;

                case change_t::swap_x:
                case change_t::swap_y:
                case change_t::swap_xy:
                    //assert(false);
                    unswap_sp(i, j, chg);
                    break;

                case change_t::reverse_x:
                case change_t::reverse_y:
                case change_t::reverse_xy:
                    unreverse_sp(i, j, chg);
                    break;

                case change_t::rotate_x:
                case change_t::rotate_y:
                case change_t::rotate_xy:
                    //assert(false);
                    unrotate_sp(i, j, chg);
                    break;

                default:
                    assert(("no match for switch", false));
                }

                get<0>(last_change_) = change_t::none;
                return ans;
            }

            // Random shuffle. This invalidates the subsequent call to rollback.
            template<typename Eng>
            void shuffle(Eng &&eng, double p_rotate = 0.5) {
                using namespace std;
                bernoulli_distribution rand_bool(p_rotate);
                for (size_t i = 0; i != this->size(); ++i)
                    if (rand_bool(eng))
                        swap(widths_[i], heights_[i]);
                std::shuffle(sp_x_.begin(), sp_x_.end(), eng);
                std::shuffle(sp_y_.begin(), sp_y_.end(), eng);
                last_change_ = forward_as_tuple(change_t::none, 0, 0);
            }

            auto size() const noexcept {
                return widths_.size();
            }

            auto empty() const noexcept {
                return widths_.empty();
            }

            friend std::ostream &operator<<(std::ostream &out, const self_t &gen) {
                return gen.print(out);
            }

            template<typename Alloc0, typename Alloc1>
            friend void unguarded_copy_unbuffered_generator(const DagPackGeneratorBase<Alloc0> &src,
                DagPackGeneratorBase<Alloc1> &dest);

        protected:

            template<typename LayoutAlloc>
            void unguarded_copy_layout_sizes(Layout<LayoutAlloc> &layout) const {
                assert(layout.size() == this->size());
                auto sz = this->size();
                std::copy(widths_.data(), widths_.data() + sz, 
                    std::addressof(*layout.widths_begin()));
                std::copy(heights_.data(), heights_.data() + sz, 
                    std::addressof(*layout.heights_begin()));
            }

            template<typename Alloc1>
            void unguarded_assign(const DagPackGeneratorBase<Alloc1> &src) {
                using namespace std;
                assert(size() == src.size());
                auto sz = size();
                copy(src.sp_x_.data(), src.sp_x_.data() + sz, sp_x_.data());
                copy(src.sp_y_.data(), src.sp_y_.data() + sz, sp_y_.data());
                copy(src.widths_.data(), src.widths_.data() + sz, widths_.data());
                copy(src.heights_.data(), src.heights_.data() + sz, heights_.data());
                last_change_ = src.last_change_;
            }

            // Implements the evaluation stage of operator(...). 
            // Requires: widths and heights between this object and layout have
            //      been synchronized.
            template<typename LayoutAlloc, typename Eng>
            std::pair<int, int> eval(Layout<LayoutAlloc> &layout,
                Eng &&eng, resource_t &res) {
                using namespace std;
                using namespace boost;
                using graph_t = adjacency_list<vecS, vecS, directedS,
                    property<vertex_distance_t, int>,
                    property<edge_weight_t, int>>;

                if (layout.empty())
                    return { 0, 0 };
                auto min_buf_size = min_buffer_size();
                if (res.size() < min_buf_size)
                    res.resize(min_buf_size);
                auto mem_src = res.data();

                // Build position maps
                auto sx = reinterpret_cast<size_t *>(mem_src);
                mem_src += size() * sizeof(size_t);
                auto sy = reinterpret_cast<size_t *>(mem_src);
                mem_src += size() * sizeof(size_t);
                detail::make_left_inverse(sp_x_.cbegin(), sp_x_.cend(), sx);
                detail::make_left_inverse(sp_y_.cbegin(), sp_y_.cend(), sy);

                const auto num_nodes = size() + 2;
                const auto src = size();
                const auto target = size() + 1;
                graph_t hg(num_nodes), vg(num_nodes);
                std::array<graph_t *, 2> graphs{ std::addressof(hg),
                    std::addressof(vg) };

                // Add the edges, note that weights are moved to out-edge.
                for (auto g : graphs)
                    for (size_t i = 0; i != size(); ++i)
                        add_edge(src, i, 0, *g);
                for (size_t i = 0; i != size(); ++i)
                    add_edge(i, target, -widths_[i], hg);
                for (size_t i = 0; i != size(); ++i)
                    add_edge(i, target, -heights_[i], vg);

                // Now directly add the horizontal constraint pairs and vertical 
                // constraint pairs to hg and vg.
                for (size_t i = 0; i < size(); ++i) {
                    for (size_t j = 0; j < size(); ++j) {
                        if (/*i != j && */sy[i] < sy[j]) {
                            if (sx[i] < sx[j]) {
                                // bb->left: i is to the left of j
                                add_edge(i, j, -widths_[i], hg);
                            } else {
                                // ab->below: i is below j
                                assert(sx[i] > sx[j]);
                                add_edge(i, j, -heights_[i], vg);
                            }
                        }
                    }
                }

                // Packing by the longest path algorithm
                std::array<int, 2> sizes;    // w, h
                std::array<decltype(layout.x_begin()), 2> dests{ layout.x_begin(),
                    layout.y_begin() };

                for (int i = 0; i != 2; ++i) {
                    auto &g = *graphs[i];
                    auto dest = dests[i];
                    property_map<graph_t, vertex_distance_t>::type
                        d_map = get(vertex_distance, g);
                    dag_shortest_paths(g, src, distance_map(d_map));
                    graph_traits<graph_t>::vertex_iterator vi, vi_end;
                    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
                        if (*vi != src && *vi != target)
                            *dest++ = -d_map[*vi];
                    sizes[i] = -d_map[target];
                }

                assert(make_pair(sizes[0], sizes[1]) == layout.get_area());
                return { sizes[0], sizes[1] };
            }

            template<typename Eng, typename ChgDist>
            bool change(Eng &&eng, ChgDist &&chg_dist) {
                bool ans = true;
                auto chg = std::forward<ChgDist>(chg_dist)(std::forward<Eng>(eng));

                switch (chg) {
                case change_t::none:
                    ans = false;
                    break;

                case change_t::rotate:
                    rotate_component(std::forward<Eng>(eng), chg);
                    break;

                case change_t::swap_x:
                case change_t::swap_y:
                case change_t::swap_xy:
                    swap_sp(std::forward<Eng>(eng), chg);
                    break;

                case change_t::reverse_x:
                case change_t::reverse_y:
                case change_t::reverse_xy:
                    reverse_sp(std::forward<Eng>(eng), chg);
                    break;

                case change_t::rotate_x:
                case change_t::rotate_y:
                case change_t::rotate_xy:
                    rotate_sp(std::forward<Eng>(eng), chg);
                    break;

                default:
                    assert(("no match for switch", false));
                }

                return ans;
            }

            template<typename Eng>
            void swap_sp(Eng &&eng, change_t chg) {
                using namespace std;
                uniform_int_distribution<size_t> rand_size_t(0, size() - 1);
                size_t i = 0, j = 0;
                while (i == j) {
                    i = rand_size_t(std::forward<Eng>(eng));
                    j = rand_size_t(std::forward<Eng>(eng));
                }

                unswap_sp(i, j, chg);
                last_change_ = forward_as_tuple(chg, i, j);
            }

            void unswap_sp(size_t i, size_t j, change_t chg) {
                using std::swap;
                if (chg == change_t::swap_x || chg == change_t::swap_xy)
                    swap(sp_x_[i], sp_x_[j]);
                if (chg == change_t::swap_y || chg == change_t::swap_xy)
                    swap(sp_y_[i], sp_y_[j]);
            }

            template<typename Eng>
            void rotate_sp(Eng &&eng, change_t chg) {
                using namespace std;
                assert(!empty());
                uniform_int_distribution<std::size_t> rand_size_t(0, size());
                size_t i = 0, j = 0;
                while (j <= i + 1) {
                    i = rand_size_t(std::forward<Eng>(eng));
                    j = rand_size_t(std::forward<Eng>(eng));
                    if (i > j)
                        swap(i, j);
                }
                
                if (chg == change_t::rotate_x || chg == change_t::rotate_xy)
                    rotate(sp_x_.data() + i, sp_x_.data() + i + 1, sp_x_.data() + j);
                if (chg == change_t::rotate_y || chg == change_t::rotate_xy)
                    rotate(sp_y_.data() + i, sp_y_.data() + i + 1, sp_y_.data() + j);

                last_change_ = forward_as_tuple(chg, i, j);
            }

            void unrotate_sp(size_t i, size_t j, change_t chg) {
                if (chg == change_t::rotate_x || chg == change_t::rotate_xy)
                    std::rotate(sp_x_.data() + i, sp_x_.data() + j - 1, sp_x_.data() + j);
                if (chg == change_t::rotate_y || chg == change_t::rotate_xy)
                    std::rotate(sp_y_.data() + i, sp_y_.data() + j - 1, sp_y_.data() + j);
            }

            template<typename Eng>
            void reverse_sp(Eng &&eng, change_t chg) {
                using namespace std;
                uniform_int_distribution<size_t> rand_size_t(0, size());
                size_t i = 0, j = 0;
                while (j <= i + 1) {
                    i = rand_size_t(std::forward<Eng>(eng));
                    j = rand_size_t(std::forward<Eng>(eng));
                    if (i > j)
                        swap(i, j);
                }

                unreverse_sp(i, j, chg);
                last_change_ = forward_as_tuple(chg, i, j);
            }

            void unreverse_sp(size_t i, size_t j, change_t chg) {
                if (chg == change_t::reverse_x || chg == change_t::reverse_xy)
                    std::reverse(sp_x_.data() + i, sp_x_.data() + j);
                if (chg == change_t::reverse_y || chg == change_t::reverse_xy)
                    std::reverse(sp_y_.data() + i, sp_y_.data() + j);
            }

            template<typename Eng>
            void rotate_component(Eng &&eng, change_t chg) {
                using namespace std;
                auto k = uniform_int_distribution<std::size_t>(0, size() - 1)(
                    std::forward<Eng>(eng));
                unrotate_component(k);
                last_change_ = forward_as_tuple(chg, k, k);
            }

            void unrotate_component(size_t k) {
                using std::swap;
                swap(widths_[k], heights_[k]);
            }

            std::ostream &print(std::ostream &out) const {
                aureliano::print(sp_x_, out) << std::endl;
                aureliano::print(sp_y_, out) << std::endl;
                return out;
            }

            // Determines size of resource_t in bytes, which is the sum of 
            // variables sx and sy. Computing wirelength can reuse the buffer.
            auto min_buffer_size() const noexcept {
                return 2 * size() * sizeof(std::size_t);
            }

            size_vector_t widths_, heights_;    // Copies of component sizes
            sequence_pair_t sp_x_, sp_y_;
            momento_t last_change_;   // One-shot info of last change 
        };

        // LCS-based sequence-pair packing generator which does not own buffer resource.
        template<typename Alloc = std::allocator<void *>>
        class LcsPackGeneratorBase : public DagPackGeneratorBase<Alloc> {
            using self_t = LcsPackGeneratorBase<Alloc>;
            using base_t = DagPackGeneratorBase<Alloc>;

        protected:
            using typename base_t::size_vector_t;
            using typename base_t::sequence_pair_t;
            using typename base_t::momento_t;

        public:
            using typename base_t::allocator_type;
            using typename base_t::resource_t;
            using typename base_t::change_t;
            using typename base_t::default_change_distribution;
            using generator_tag = UnbufferedGeneratorTag;

            using base_t::DagPackGeneratorBase;
        
        protected:
            // Implements the evaluation stage of operator(...).
            template<typename LayoutAlloc, typename Eng>
            std::pair<int, int> eval(Layout<LayoutAlloc> &layout,
                Eng &&eng, resource_t &res) {
                using namespace std;

                // Deal with auxilary buffer.
                auto min_buffer_size = base_t::min_buffer_size();
                if (res.size() < min_buffer_size)
                    res.resize(min_buffer_size);
                auto mem_src = res.data();
                auto match = reinterpret_cast<size_t *>(mem_src);
                mem_src += this->size() * sizeof(size_t);
                auto buffer = reinterpret_cast<size_t *>(mem_src);
                mem_src += this->size() * sizeof(size_t);

                // Evaluate current state.
                using map_alloc_t = typename std::allocator_traits<allocator_type>
                    ::template rebind_alloc<std::pair<const ptrdiff_t, ptrdiff_t>>;
                std::map<ptrdiff_t, ptrdiff_t, less<ptrdiff_t>, map_alloc_t>
                    pq(std::less<ptrdiff_t>(), this->widths_.get_allocator());  
                auto w = detail::eval_sp2(this->sp_y_.cbegin(), this->sp_y_.cend(),
                    this->sp_x_.cbegin(), this->widths_.cbegin(), layout.x_begin(),
                    buffer, match, pq);
                auto h = detail::eval_sp2(this->sp_y_.cbegin(), this->sp_y_.cend(),
                    this->sp_x_.crbegin(), this->heights_.cbegin(), layout.y_begin(),
                    buffer, match, pq);

                assert(match == reinterpret_cast<size_t *>(res.data()));
                auto sln_area = make_pair(static_cast<int>(w), static_cast<int>(h));
#ifndef NDEBUG
                auto layout_area = layout.get_area();
                if (sln_area != layout_area) {
                    using namespace seqpair::io;
                    cerr << "sln_area: " << sln_area << endl;
                    cerr << "layout_area: " << layout_area << endl;
                    assert(sln_area == layout_area);
                }
#endif  
                return sln_area;
            }
        };

        template<typename Alloc0, typename Alloc1>
        void unguarded_copy_generator(const DagPackGeneratorBase<Alloc0> &src,
            DagPackGeneratorBase<Alloc1> &dest) {
            unguarded_copy_unbuffered_generator(src, dest);
        }

        template<typename Alloc0, typename Alloc1>
        void unguarded_copy_unbuffered_generator(const DagPackGeneratorBase<Alloc0> &src,
            DagPackGeneratorBase<Alloc1> &dest) {
            dest.unguarded_assign(src);
        }

        // Pack generator which owns resource made by its base class.
        template<typename BaseGenerator>
        class BufferedPackGenerator : public BaseGenerator {
            using self_t = BufferedPackGenerator<BaseGenerator>;
            using base_t = BaseGenerator;

        protected:
            using typename base_t::resource_t;
            using typename base_t::size_vector_t;
            using typename base_t::sequence_pair_t;
            using typename base_t::momento_t;

        public:
            using typename base_t::allocator_type;
            using typename base_t::change_t;
            using typename base_t::default_change_distribution;
            using unbuffered_generator_t = base_t;
            using buffered_generator_t = self_t;
            using generator_tag = BufferedGeneratorTag;

            BufferedPackGenerator() : BufferedPackGenerator(allocator_type()) { }

            explicit BufferedPackGenerator(const allocator_type &alloc) : 
                base_t(alloc), resource_(base_t::make_resource()) { }

            template<typename Cont0, typename Cont1, typename Eng>
                BufferedPackGenerator(Cont0 &&widths, Cont1 &&heights,
                    Eng &&eng, const allocator_type &alloc = allocator_type()) :
                base_t(std::forward<Cont0>(widths), std::forward<Cont1>(heights),
                    std::forward<Eng>(eng), alloc),
                resource_(base_t::make_resource()) { }

            template<typename LayoutAlloc, typename Eng,
                typename ChgDist = default_change_distribution, typename OtherAlloc>
                std::pair<int, int> operator()(Layout<LayoutAlloc> &layout,
                    Eng &&eng, ChgDist &&chg_dist) {
                return base_t::operator()(layout, std::forward<Eng>(eng), resource_,
                    std::forward<ChgDist>(chg_dist));
            }

            template<typename BaseGenerator0, typename BaseGenerator1>
            friend void detail::unguarded_copy_generator(
                const BufferedPackGenerator<BaseGenerator0> &src,
                BufferedPackGenerator<BaseGenerator1> &dest);

        protected:
            using base_t::make_resource;

            resource_t resource_;
        };
    }

    template<typename Alloc = std::allocator<void *>>
    using DagPackGenerator = detail::BufferedPackGenerator<detail::DagPackGeneratorBase<Alloc>>;

    template<typename Alloc = std::allocator<void *>>
    using LcsPackGenerator = detail::BufferedPackGenerator<detail::LcsPackGeneratorBase<Alloc>>;

    using seqpair::detail::PackGeneratorBase;

    namespace detail {
        template<typename BaseGenerator0, typename BaseGenerator1>
        void unguarded_copy_generator(const BufferedPackGenerator<BaseGenerator0> &src,
            BufferedPackGenerator<BaseGenerator1> &dest) {
            unguarded_copy_unbuffered_generator(src, dest);
            dest.resource_ = src.resource_;
        }
    }
}
