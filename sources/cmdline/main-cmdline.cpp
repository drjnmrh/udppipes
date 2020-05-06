#include <atomic>
#include <iostream>
#include <list>
#include <mutex>
#include <set>

#include <csignal>

#include "commons/logger.hpp"

#include "testapi.hpp"


#define DECLARE_SUIT(SuitName) \
    EXTERN_TEST void Collect ## SuitName ## Tests(TestDesc**, int*)


#define ENABLE_SUIT(TestsListVar, SuitName) \
    do{ \
        TestDesc* pDescs; \
        int nbDescs; \
        Collect ## SuitName ## Tests(&pDescs, &nbDescs); \
        for (int i = 0; i < nbDescs; ++i) { \
            (TestsListVar).push_back(pDescs[i]); \
        } \
    } while(false)


DECLARE_SUIT(Threader);
DECLARE_SUIT(UdpEngine);


int main(int argc, char** argv) {

    LOGI << "UDP Pipes testing";

    std::list<TestDesc> allTests;

    ENABLE_SUIT(allTests, Threader);
    ENABLE_SUIT(allTests, UdpEngine);

    LOGI << "Running " << allTests.size() << " tests:";

    int testIndex = 0;
    for(auto &td : allTests) {
        std::chrono::steady_clock::time_point startTp = std::chrono::steady_clock::now();

        for(int i = 0; i < td.mNumIterations; ++i) {
            if (!td.mMethod())
                return 1;
        }

        std::chrono::steady_clock::time_point finishTp = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsedMs = (finishTp - startTp) * 1000.f;

        LOGI << "[" << ++testIndex << "/" << allTests.size() << "] " << td.mName << " finished in " << elapsedMs.count() << " ms.";
    }

    return 0;
}
