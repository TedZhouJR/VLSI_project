// xseqpair.h: top-level tools.

#pragma once
#include <iostream>
#include <utility>
#include <random>

#define SEQPAIR_RANDOM_SEED() (std::random_device{}())

#ifndef NDEBUG
#define SEQPAIR_DEBUG_RANDOM_SEED() (0)
#else
#define SEQPAIR_DEBUG_RANDOM_SEED() (std::random_device{}())
#endif

#define SEQPAIR_IO_BE_INLINE  

namespace seqpair {
    namespace io {
        // Note: maybe can be generalized to tuples.
        template<typename Ty0, typename Ty1>
        std::ostream &operator<<(std::ostream &out, 
            const std::pair<Ty0, Ty1> &p) {
            return out << "(" << p.first << ", " << p.second << ")";
        }
    }
}


