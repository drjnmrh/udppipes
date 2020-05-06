#ifndef UDP_UDPCMD_TESTAPI_TESTAPI_HPP_
#define UDP_UDPCMD_TESTAPI_TESTAPI_HPP_


#include "commons/debug.h"

#include "commons/logger.hpp"


#if defined(__cplusplus)
#   define EXTERN_TEST extern "C"
#else
#   define EXTERN_TEST extern
#endif


struct TestDesc {
    using TestMethod = bool (*) ();

    TestMethod mMethod;
    char mName[128];
    int mNumIterations;
};


#define START_TEST_SUIT_DECLARATION(SuitName) \
    static TestDesc s ## SuitName ## Tests[] = {

#define DECLARE_TEST(Method) \
    TestDesc{ .mMethod = &(Method), .mName = # Method, .mNumIterations = 1024 },

#define DECLARE_TEST_ITERATED(Method, NumIterations) \
    TestDesc{ .mMethod = &(Method), .mName = # Method, .mNumIterations = (NumIterations) },

#define FINISH_TEST_SUIT_DECLARATION(SuitName) \
    }; \
    EXTERN_TEST void Collect ## SuitName ## Tests(TestDesc** pDescs, int* outDescsNum) { \
        *pDescs = s ## SuitName ## Tests; \
        *outDescsNum = sizeof(s ## SuitName ## Tests) / sizeof(s ## SuitName ## Tests[0]); \
    }


#define CHECK_EQUAL(Expr, Value) \
    do{   \
        auto exprValue = (Expr);  \
        if (exprValue != (Value)) {    \
            LOGE << "CHECK FAILED: " << exprValue << " != " << (Value) << " at\n\t" << __FILE__ << " [" << __LINE__ << ":" << __COUNTER__ << "] in\n\t\t" << FUNCNAME; \
            RaiseSoftbreak(); \
            return false; \
        } \
    }while(false)


#define CHECK_GREATER(Expr, Value) \
    do { \
        auto exprValue = (Expr); \
        if (exprValue <= (Value)) { \
            LOGE << "CHECK FAILED: " << exprValue << " <= " << (Value) << " in " << __FILE__ << " [" << __LINE__ << ":" << __COUNTER__ << "] in\n\t" << FUNCNAME; \
            RaiseSoftbreak(); \
            return false; \
        } \
    }while(false)


#define CHECK_LESS(Expr, Value) \
    do { \
        auto exprValue = (Expr); \
        if (exprValue >= (Value)) { \
            LOGE << "CHECK FAILED: " << exprValue << " >= " << (Value) << " in " << __FILE__ << " [" << __LINE__ << ":" << __COUNTER__ << "] in\n\t" << FUNCNAME; \
            RaiseSoftbreak(); \
            return false; \
        } \
    }while(false)


#define CHECK_TRUE(Expr) \
    do { \
        auto exprValue = (Expr); \
        if (!exprValue) { \
            LOGE << "CHECK FAILED: expression is false (must be true) in " << __FILE__ << " [" << __LINE__ << ":" << __COUNTER__ << "] in\n\t" << FUNCNAME; \
            RaiseSoftbreak(); \
            return false; \
        } \
    }while(false)


#define CHECK_FALSE(Expr) \
    do { \
        auto exprValue = (Expr); \
        if (!!exprValue) { \
            LOGE << "CHECK FAILED: expression is true (must be false) in " << __FILE__ << " [" << __LINE__ << ":" << __COUNTER__ << "] in\n\t" << FUNCNAME; \
            RaiseSoftbreak(); \
            return false; \
        } \
    }while(false)


#endif//UDP_UDPCMD_TESTAPI_TESTAPI_HPP_
