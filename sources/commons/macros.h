#ifndef UDP_COMMONS_MACROS_H_
#define UDP_COMMONS_MACROS_H_


#include "commons/debug.h"


#define HARDBREAK RaiseHardbreak(__FILE__, __LINE__, __COUNTER__)

#define UNUSED(Var) (void)Var


#if defined(__cplusplus)

#   define EXTERN extern "C"

#   define NOCOPY(ClassName)                                                       \
        ClassName(const ClassName&) = delete;                                       \
        ClassName& operator = (const ClassName&) = delete;

#   define NOMOVE(ClassName)                                                       \
        ClassName(ClassName&&) = delete;                                            \
        ClassName& operator = (ClassName&&) = delete;


#if defined(_MSC_VER)
#   if defined(_CPPUNWIND)
#       define TRY try
#       define CATCHALL catch(...)
#       define CATCH(ExceptionType, ExceptionVar) catch(ExceptionType& ExceptionVar)
#   else
        namespace { struct DummyException { const char* what() const noexcept { return ""; } }; }
#       define TRY
#       define CATCHALL if (false)
#       define CATCH(ExceptionType, ExceptionVar) for (DummyException ExceptionVar; false;)
#   endif
#else
#   ifdef __EXCEPTIONS
#       define TRY try
#       define CATCHALL catch(...)
#       define CATCH(ExceptionType, ExceptionVar) catch(ExceptionType& ExceptionVar)
#   else
        namespace { struct DummyException { const char* what() const noexcept { return ""; } }; }
#       define TRY
#       define CATCHALL if (false)
#       define CATCH(ExceptionType, ExceptionVar) for (DummyException ExceptionVar; false;)
#   endif
#endif


#define TRY_LOCKED1(VarName) \
    TRY { std::scoped_lock<decltype(VarName ## M)> sl(VarName ## M);

#define TRY_LOCKED2(VarName1, VarName2) \
    TRY { std::scoped_lock sl(VarName1 ## M, VarName2 ## M);

#define UNLOCK } CATCHALL { HARDBREAK; }


#define TRY_LOCKEDX(x, A, B, FUNC, ...) FUNC

#define TRY_LOCKED(...) \
    TRY_LOCKEDX(, ##__VA_ARGS__, TRY_LOCKED2(__VA_ARGS__), TRY_LOCKED1(__VA_ARGS__))


#else

#   define EXTERN extern

#endif



#if defined(_MSC_VER)


#include <intrin.h>


#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanReverse64)


#define BUILTIN_MSNZB32(Value, ResultPtr) \
    do { unsigned long r; _BitScanReverse(&r, (unsigned long)(Value)); *ResultPtr = r; } while(0)

#define BUILTIN_MSNZB64(Value, ResultPtr) \
    do { unsigned long r; _BitScanReverse64(&r, (unsigned long long)(Value)); *ResultPtr = r; } while(0)


#else


#define BUILTIN_MSNZB32(Value, ResultPtr) \
    do { *ResultPtr = 31 - __builtin_clz((unsigned int)(Value)); } while(0)

#define BUILTIN_MSNZB64(Value, ResultPtr) \
    do { *ResultPtr = 63 - __builtin_clzll((unsigned long long)(Value)); } while(0)


#endif


#define CACHELINE_SIZE_IN_BYTES 64


#if defined(__APPLE__)

#   define CACHELINE(Index) \
    [[maybe_unused]] char __pad ## Index [CACHELINE_SIZE_IN_BYTES]

#else

#   define CACHELINE(Index) \
    char __pad ## Index ## [CACHELINE_SIZE_IN_BYTES]

#endif


#if !defined(__PRETTY_FUNCTION__)
#   if defined(__FUNCSIG__)
#       define FUNCNAME __FUNCSIG__
#   else
#       define FUNCNAME __func__
#   endif
#else
#   define FUNCNAME __PRETTY_FUNCTION__
#endif


#endif//UDP_COMMONS_MACROS_H_
