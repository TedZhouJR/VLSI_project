// timeit.h: timing tools inspired by Python.
// Author: LYL (Aureliano Lee)

#pragma once

#include <chrono>
#include <type_traits>
#include "xaureliano.h"

AURELIANO_BEGIN
// Records total runtime of running func(args...).
// Returns: runtime measured with std::chrono::high_resolution_clock.
// Note: The function only participates in overload resolution when 
//		 func is not arithmetic. 
//		 With c++17, std::is_invocable can tackle this better.
template<typename Func, typename... Types>
inline typename std::enable_if<!std::is_arithmetic<Func>::value,
    typename std::chrono::high_resolution_clock::duration>::type
    timeit(Func &&func, Types &&...args) {
    auto t0 = std::chrono::high_resolution_clock::now();
    std::forward<Func>(func)(std::forward<Types>(args)...);
    auto t1 = std::chrono::high_resolution_clock::now();
    return t1 - t0;
}

// Records total runtime of running func(args...) for test_times.
// Returns: runtime measured with std::chrono::high_resolution_clock.
template<typename Func, typename... Types>
inline typename std::chrono::high_resolution_clock::duration
timeit(unsigned long long test_times, Func &&func, Types &&...args) {
    auto t0 = std::chrono::high_resolution_clock::now();
    while (test_times--)
        std::forward<Func>(func)(std::forward<Types>(args)...);
    auto t1 = std::chrono::high_resolution_clock::now();
    return t1 - t0;
}
AURELIANO_END
