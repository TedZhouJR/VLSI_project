// toolbox.h: small tools
// Author: LYL (Aureliano Lee)

#pragma once

#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include "xaureliano.h"

#define AURELIANO_VA_ARGS_SIZE(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value
#define AURELIANO_GET_MACRO3(_0, _1, _2, NAME,...) NAME
#define AURELIANO_EXPAND(X) X

#define AURELIANO_PRINT_EXPRS_IMPL3(EXPRS, OUT, DELIM) (OUT << #EXPRS << DELIM << (EXPRS))
#define AURELIANO_PRINT_EXPRS_IMPL2(EXPRS, OUT) AURELIANO_PRINT_EXPRS_IMPL3(EXPRS, OUT, " == ")
#define AURELIANO_PRINT_EXPRS_IMPL1(EXPRS) AURELIANO_PRINT_EXPRS_IMPL2(EXPRS, std::cout)

// Params: [EXPRS[, OUT=std::cout[, DELIM=" == "]]]
// Value: Reference to OUT
#define AURELIANO_PRINT_EXPRS(...) AURELIANO_EXPAND(AURELIANO_GET_MACRO3(__VA_ARGS__, \
AURELIANO_PRINT_EXPRS_IMPL3, AURELIANO_PRINT_EXPRS_IMPL2, AURELIANO_PRINT_EXPRS_IMPL1)(__VA_ARGS__))

// Params: [OUT=std::cout]
// Value: Reference to OUT
#define AURELIANO_PRINT_POSITION(...) AURELIANO detail::print_position(__FILE__, __LINE__, \
__FUNCTION__, __VA_ARGS__)


AURELIANO_BEGIN

namespace detail {

    // Prints current position.
    template<typename Stream>
    inline decltype(auto) print_position(const char *file, int line,
        const char *func, Stream &&out) {
        return std::forward<Stream>(out) << file << "(" << line << "), " << func; 
    }

    // Prints current position.
     inline decltype(auto) print_position(const char *file, int line,
        const char *func) {
        return print_position(file, line, func, std::cout);
    }

#if defined __cplusplus  && __cplusplus >= 201703L

    template<typename Alloc>
    inline void swap_container_allocators(Alloc &left, Alloc &right, std::true_type)
        noexcept(std::is_nothrow_swappable_v<Alloc>) {
        using std::swap;
        swap(left, right);
    }

    template<typename Alloc>
    void swap_container_allocators(Alloc &left, Alloc &right, std::false_type) noexcept {}

#endif

}  

// Prints [first, last).
template<typename InIt>
inline std::ostream &print(InIt first, InIt last, 
    std::ostream &out = std::cout) {
    while (first != last)
        out << *first++ << " ";
    return out;
}

// Prints container.
template<typename Container>
inline std::ostream &print(Container &&container, 
    std::ostream &out = std::cout) {
    return print(std::begin(std::forward<Container>(container)), 
        std::end(std::forward<Container>(container)));
}

// Implements std::void_t (C++17).
template<typename ...>
using Void_t = void;

// Checks whether InIt is iterator
// by checking typename InIt::iterator_category
template<typename InIt, typename = void>
struct IsIterator : public std::false_type {};

// Checks whether InIt is iterator.
// by checking typename InIt::iterator_category
template<typename InIt>
struct IsIterator<InIt, Void_t<
    typename std::iterator_traits<InIt>::iterator_category
    > > : public std::true_type {};

#if defined __cplusplus  && __cplusplus >= 201703L

template<typename Alloc>
using is_container_allocator_nothrow_swappable = std::disjunction<typename std::allocator_traits<Alloc>::propagate_on_container_swap, typename std::is_nothrow_swappable<Alloc>>;

template<typename Alloc>
inline constexpr bool is_container_allocator_nothrow_swappable_v = is_container_allocator_nothrow_swappable<Alloc>::value;

template<typename Alloc>
void swap_container_allocators(Alloc &left, Alloc &right)
noexcept(std::allocator_traits<Alloc>::propagate_on_container_swap::value ||
    std::is_nothrow_swappable_v<Alloc>) {
    typename std::allocator_traits<Alloc>::propagate_on_container_swap tag;
    detail::swap_container_allocators(left, right, tag);
}

#endif

AURELIANO_END
