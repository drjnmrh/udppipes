#ifndef UDP_COMMONS_UTILS_HPP_
#define UDP_COMMONS_UTILS_HPP_


#include <cstdint>

#include <type_traits>

#include "commons/macros.h"



namespace udp { ;


using InvocationBlock = void (*) (void);


void dispatch_once(intptr_t* flag, InvocationBlock block);


template < typename IntegerType >
static inline IntegerType ToPowerOf2(IntegerType value) noexcept {

    static_assert(std::is_arithmetic_v<IntegerType>);

    if (value == 0 || (value & (value - 1)) == 0)
        return value;

    if constexpr(sizeof(IntegerType) == sizeof(unsigned long long)) {
        unsigned int r;
        BUILTIN_MSNZB64(value, &r);

        return 1 << (r + 1);
    } else {
        unsigned int r;
        BUILTIN_MSNZB32(value, &r);

        return 1 << (r + 1);
    }
}


}


#endif//UDP_COMMONS_UTILS_HPP_
