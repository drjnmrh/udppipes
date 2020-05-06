#include <atomic>
#include <chrono>
#include <list>
#include <memory>
#include <thread>

#include "commons/macros.h"

#include "sockets/udpengine.hpp"

#include "testapi.hpp"


#pragma mark - Tests Declarations

bool test__udp_sockets_UdpEngine__correctness_start_stop();
bool test__udp_sockets_UdpEngine__correctness_singlethread_multi_start();
bool test__udp_sockets_UdpEngine__correctness_singlethread_multi_stop();
bool test__udp_sockets_UdpEngine__correctness_singlethread_multi_start_stop();
bool test__udp_sockets_UdpEngine__correctness_multithread_multi_start();
bool test__udp_sockets_UdpEngine__correctness_multithread_multi_stop();
bool test__udp_sockets_UdpEngine__correctness_multithread_multi_start_stop();
bool test__udp_sockets_UdpEngine__lifeness_multithread_multi_start_stop();
bool test__udp_sockets_UdpEngine__correctness_singlethread_attach_detach_users();
bool test__udp_sockets_UdpEngine__correctness_singlethread_send_recieve_dgram();
bool test__udp_sockets_UdpEngine__correctness_singlethread_send_recieve_answer_dgram();
bool test__udp_sockets_UdpEngine__lifeness_multithread_send_recieve_dgrams();


START_TEST_SUIT_DECLARATION(UdpEngine)
    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_start_stop)

    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_singlethread_multi_start)
    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_singlethread_multi_stop)
    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_singlethread_multi_start_stop)

    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_multithread_multi_start)
    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_multithread_multi_stop)
    DECLARE_TEST(test__udp_sockets_UdpEngine__correctness_multithread_multi_start_stop)

    DECLARE_TEST(test__udp_sockets_UdpEngine__lifeness_multithread_multi_start_stop)

    DECLARE_TEST_ITERATED(test__udp_sockets_UdpEngine__correctness_singlethread_attach_detach_users, 1)

    DECLARE_TEST_ITERATED(test__udp_sockets_UdpEngine__correctness_singlethread_send_recieve_dgram, 1)
    DECLARE_TEST_ITERATED(test__udp_sockets_UdpEngine__correctness_singlethread_send_recieve_answer_dgram, 1)
    DECLARE_TEST_ITERATED(test__udp_sockets_UdpEngine__lifeness_multithread_send_recieve_dgrams, 1)
FINISH_TEST_SUIT_DECLARATION(UdpEngine)


#pragma mark - Tests Utils

using namespace udp::sockets;


using std_clock = std::chrono::steady_clock;


namespace {


class TestUdpEngine : public priv::UdpEngine {
public:
    TestUdpEngine() noexcept : UdpEngine() {}
};


class TestUdpUser : public priv::IUdpUser {
public:
    void setUp( int socketId
              , priv::UdpDgramQueue::SPtr pInputQueue
              , priv::UdpDgramQueue::SPtr pOutputQueue ) noexcept override
    {
        TRY_LOCKED(_socketsList) {
            _socketsList.push_back(socketId);
        } UNLOCK;

        _pInputQueue = pInputQueue;
        _pOutputQueue = pOutputQueue;
    }

    void notifyInvalid() noexcept override {

        TRY_LOCKED(_socketsList) {
            _socketsList.push_back(-1);
        } UNLOCK;
    }

    const std::list<int>& socketsList() const noexcept { return _socketsList; }

    priv::UdpDgramQueue* input() const noexcept { return _pInputQueue.get(); }
    priv::UdpDgramQueue* output() const noexcept { return _pOutputQueue.get(); }

private:

    std::mutex _socketsListM;
    std::list<int> _socketsList;

    priv::UdpDgramQueue::SPtr _pInputQueue;
    priv::UdpDgramQueue::SPtr _pOutputQueue;
};


struct StartStopThreadDelegate {
    enum StateType { eState_Down, eState_Work, eState_Abort };

    struct Context {
        CACHELINE(0);

        std::atomic<bool> mStartFlag;

        CACHELINE(1);

        std::mutex mActiveFlagM;
        bool mActiveFlag;

        CACHELINE(2);
    };

    CACHELINE(0);

    std::atomic<StateType> mState;

    CACHELINE(1);

    std::unique_ptr<std::thread> mThreadPtr;
    priv::UdpEngine* mEnginePtr;

    int mNumberOfAttempts;
    bool mIsSuccessful;

    CACHELINE(2);

    ~StartStopThreadDelegate() noexcept {

        StateType expected = eState_Work;
        if (mState.compare_exchange_strong(expected, eState_Abort)) {
            while(true) {
                if (eState_Down == mState.load())
                    break;

                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        }
    }

    bool runAttemptsToStart(Context*) {

        UdpResult udpres;
        for (; mNumberOfAttempts > 0; --mNumberOfAttempts) {
            if (eState_Abort == mState.load(std::memory_order_relaxed))
                return false;

            udpres = mEnginePtr->startUp();
            CHECK_EQUAL(udpres, eUdpResult_Already);

            int delayInNs = std::rand() % 64 + 1;
            std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
        }

        return true;
    }

    bool runAttemptsToStop(Context*) {

        UdpResult udpres;
        for (; mNumberOfAttempts > 0; --mNumberOfAttempts) {
            if (eState_Abort == mState.load(std::memory_order_relaxed))
                return false;

            udpres = mEnginePtr->tearDown();
            CHECK_EQUAL(udpres, eUdpResult_Already);

            int delayInNs = std::rand() % 64 + 1;
            std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
        }

        return true;
    }

    bool runAttemptsToStartOrStop(Context* pCtx) {

        UdpResult udpres;

        for (; mNumberOfAttempts > 0; --mNumberOfAttempts) {
            int op = std::rand() % 2;
            if (0 == op) {
                TRY_LOCKED(pCtx->mActiveFlag) {
                    udpres = mEnginePtr->startUp();
                    if (!pCtx->mActiveFlag) {
                        CHECK_EQUAL(udpres, eUdpResult_Ok);
                    } else {
                        CHECK_EQUAL(udpres, eUdpResult_Already);
                    }
                    pCtx->mActiveFlag = true;
                } UNLOCK;
            } else {
                TRY_LOCKED(pCtx->mActiveFlag) {
                    udpres = mEnginePtr->tearDown();
                    if (pCtx->mActiveFlag) {
                        CHECK_EQUAL(udpres, eUdpResult_Ok);
                    } else {
                        CHECK_EQUAL(udpres, eUdpResult_Already);
                    }
                    pCtx->mActiveFlag = false;
                } UNLOCK;
            }

            int delayInNs = std::rand() % 64 + 1;
            std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
        }

        return true;
    }

    bool runAttemptsToStartOrStopNoCheck(Context* pCtx) {

        UdpResult udpres;

        for (; mNumberOfAttempts > 0; --mNumberOfAttempts) {
            int op = std::rand() % 2;
            if (0 == op) {
                udpres = mEnginePtr->startUp();
            } else {
                udpres = mEnginePtr->tearDown();
            }

            CHECK_TRUE(udpres == eUdpResult_Ok || udpres == eUdpResult_Already);

            int delayInNs = std::rand() % 64 + 1;
            std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
        }

        return true;
    }

    static void TryToStop(StartStopThreadDelegate* pSelf, Context* pContext, bool (StartStopThreadDelegate::*pMethod)(Context*)) {

        pSelf->mState.store(eState_Work);

        while(true) {
            if (pContext->mStartFlag.load(std::memory_order_acquire)) {
                break;
            }

            if (eState_Abort == pSelf->mState.load(std::memory_order_relaxed)) {
               if (pSelf->mThreadPtr)
                   pSelf->mThreadPtr->detach();

               pSelf->mState.store(eState_Down);

               return;
            }

            std::this_thread::yield();
        }

        pSelf->mIsSuccessful = (pSelf->*pMethod)(pContext);

        if (pSelf->mThreadPtr)
            pSelf->mThreadPtr->detach();

        pSelf->mState.store(eState_Down);
    }
};


UdpDgram GenerateRandomDgram(size_t minSize, size_t maxSize, UdpAddress source) {

    size_t szData = minSize + (size_t)(std::rand() % (maxSize - minSize + 1));
    assert(szData >= minSize);
    assert(szData <= maxSize);

    std::unique_ptr<uint8_t[]> pData = std::make_unique<uint8_t[]>(szData);
    for (size_t i = 0; i < szData; ++i) {
        pData[i] = (uint8_t)(std::rand() % 256);
    }

    return UdpDgram(std::move(source), std::move(pData), szData);
}


struct SendRecieveThreadDelegate {
    enum StateType { eState_Down, eState_Work, eState_Abort };

    struct Context {
        CACHELINE(0);

        std::atomic<bool> mStartFlag;

        CACHELINE(1);

        TestUdpUser *mServerPtr, *mClientPtr;

        CACHELINE(2);
    };

    CACHELINE(0);

    std::atomic<StateType> mState;

    CACHELINE(1);

    std::unique_ptr<std::thread> mThreadPtr;

    size_t mNumberSentDgrams;
    size_t mNumberRecievedDgrams;
    size_t mNumberRecievedServerDgrams;
    bool mIsSuccessful;

    CACHELINE(2);

    ~SendRecieveThreadDelegate() noexcept {

        StateType expected = eState_Work;
        if (mState.compare_exchange_strong(expected, eState_Abort)) {
            while(true) {
                if (eState_Down == mState.load())
                    break;

                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        }
    }

    bool doRandomSendsRecievesStep(Context* pCtx) {

        assert(!!pCtx);

        int user = std::rand() % 10;
        if (user <= 5) {
            UdpDgram recieved;

            while (pCtx->mServerPtr->input()->dequeue(recieved)) {
                ++mNumberRecievedDgrams;

                int nbAnswers = std::rand() % 8 + 1;
                for (int i = 0; i < nbAnswers; ++i) {
                    bool enqueued = pCtx->mServerPtr->output()->enqueue(recieved.clone());
                    if (!enqueued) {
                        // queue is full - next step will be more lucky
                        LOGW << "server buffer overflow";
                        return true;
                    }
                    ++mNumberSentDgrams;
                }
            }
        } else {
            UdpDgram recieved;
            while (pCtx->mClientPtr->input()->dequeue(recieved)) {
                ++mNumberRecievedServerDgrams;
            }

            if (pCtx->mClientPtr->output()->enqueue(GenerateRandomDgram(8, 16, UdpAddress()))) {
                ++mNumberSentDgrams;
            }
        }

        return true;
    }


    static void DoDelegateWork(SendRecieveThreadDelegate* pSelf, Context* pContext, bool (SendRecieveThreadDelegate::*pMethod)(Context*)) {

        pSelf->mState.store(eState_Work);

        while(true) {
            if (pContext->mStartFlag.load(std::memory_order_acquire)) {
                break;
            }

            if (eState_Abort == pSelf->mState.load(std::memory_order_relaxed)) {
               if (pSelf->mThreadPtr)
                   pSelf->mThreadPtr->detach();

               pSelf->mState.store(eState_Down);

               return;
            }

            std::this_thread::yield();
        }

        while (true) {
            if (eState_Abort == pSelf->mState.load(std::memory_order_relaxed)) {
                break;
            }

            bool res = (pSelf->*pMethod)(pContext);
            if (!res) {
                pSelf->mIsSuccessful = false;
                break;
            }

            int delayInNs = std::rand() % 32 + 32;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayInNs));
        }

        if (pSelf->mThreadPtr)
            pSelf->mThreadPtr->detach();

        pSelf->mState.store(eState_Down);
    }
};



}


#pragma mark - startUp/tearDown

bool test__udp_sockets_UdpEngine__correctness_start_stop() {

    TestUdpEngine engine;

    UdpResult udpres = engine.startUp();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    udpres = engine.tearDown();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_singlethread_multi_start() {

    TestUdpEngine engine;

    UdpResult udpres = engine.startUp();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    for (int nbAttempts = std::rand() % 17 + 16; nbAttempts > 0; --nbAttempts) {
        udpres = engine.startUp();
        CHECK_EQUAL(udpres, eUdpResult_Already);

        int delayInNs = std::rand() % 64 + 1;
        std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
    }

    udpres = engine.tearDown();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_singlethread_multi_stop() {

    TestUdpEngine engine;

    UdpResult udpres = engine.startUp();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    udpres = engine.tearDown();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    for (int nbAttempts = std::rand() % 17 + 16; nbAttempts > 0; --nbAttempts) {
        udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Already);

        int delayInNs = std::rand() % 64 + 1;
        std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
    }

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_singlethread_multi_start_stop() {

    TestUdpEngine engine;

    bool isActive = false;
    UdpResult udpres;

    for (int nbAttempts = std::rand() % 33 + 16; nbAttempts > 0; --nbAttempts) {
        int op = std::rand() % 2;
        if (0 == op) {
            udpres = engine.startUp();
            if (isActive) {
                CHECK_EQUAL(udpres, eUdpResult_Already);
            } else {
                CHECK_EQUAL(udpres, eUdpResult_Ok);
            }
            isActive = true;
        } else {
            udpres = engine.tearDown();
            if (isActive) {
                CHECK_EQUAL(udpres, eUdpResult_Ok);
            } else {
                CHECK_EQUAL(udpres, eUdpResult_Already);
            }
            isActive = false;
        }

        int delayInNs = std::rand() % 64 + 1;
        std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
    }

    if (isActive) {
        udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_multithread_multi_start() {

    TestUdpEngine engine;

    StartStopThreadDelegate::Context ctx;
    ctx.mStartFlag.store(false, std::memory_order_relaxed);
    ctx.mActiveFlag = true;

    static const int sNumberOfThreads = 32;

    StartStopThreadDelegate delegates[sNumberOfThreads];
    for (int i = 0; i < sNumberOfThreads; ++i) {
        delegates[i].mState.store(StartStopThreadDelegate::eState_Down, std::memory_order_relaxed);
        delegates[i].mEnginePtr = &engine;
        delegates[i].mNumberOfAttempts = std::rand() % 17 + 16;
        delegates[i].mIsSuccessful = true;
        delegates[i].mThreadPtr = std::make_unique<std::thread>(StartStopThreadDelegate::TryToStop, &delegates[i], &ctx, &StartStopThreadDelegate::runAttemptsToStart);
    }

    UdpResult udpres = engine.startUp();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    ctx.mStartFlag.store(true, std::memory_order_release);

    while(true) {
        bool allFinished = true;
        for (int i = 0; i < sNumberOfThreads; ++i) {
            if (StartStopThreadDelegate::eState_Down != delegates[i].mState.load(std::memory_order_relaxed)) {
                allFinished = false;
                break;
            }
        }

        if (allFinished)
            break;

        std::this_thread::yield();
    }

    std::atomic_thread_fence(std::memory_order_acquire);

    for (int i = 0; i < sNumberOfThreads; ++i) {
        if (!delegates[i].mIsSuccessful)
            return false;
    }

    udpres = engine.tearDown();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_multithread_multi_stop() {

    TestUdpEngine engine;

    StartStopThreadDelegate::Context ctx;
    ctx.mStartFlag.store(false, std::memory_order_relaxed);
    ctx.mActiveFlag = false;

    static const int sNumberOfThreads = 32;

    StartStopThreadDelegate delegates[sNumberOfThreads];
    for (int i = 0; i < sNumberOfThreads; ++i) {
        delegates[i].mState.store(StartStopThreadDelegate::eState_Down, std::memory_order_relaxed);
        delegates[i].mEnginePtr = &engine;
        delegates[i].mNumberOfAttempts = std::rand() % 17 + 16;
        delegates[i].mIsSuccessful = true;
        delegates[i].mThreadPtr = std::make_unique<std::thread>(StartStopThreadDelegate::TryToStop, &delegates[i], &ctx, &StartStopThreadDelegate::runAttemptsToStop);
    }

    UdpResult udpres = engine.startUp();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    udpres = engine.tearDown();
    CHECK_EQUAL(udpres, eUdpResult_Ok);

    ctx.mStartFlag.store(true, std::memory_order_release);

    while(true) {
        bool allFinished = true;
        for (int i = 0; i < sNumberOfThreads; ++i) {
            if (StartStopThreadDelegate::eState_Down != delegates[i].mState.load(std::memory_order_relaxed)) {
                allFinished = false;
                break;
            }
        }

        if (allFinished)
            break;

        std::this_thread::yield();
    }

    std::atomic_thread_fence(std::memory_order_acquire);

    for (int i = 0; i < sNumberOfThreads; ++i) {
        if (!delegates[i].mIsSuccessful)
            return false;
    }

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_multithread_multi_start_stop() {

    TestUdpEngine engine;

    StartStopThreadDelegate::Context ctx;
    ctx.mStartFlag.store(false, std::memory_order_relaxed);
    ctx.mActiveFlag = false;

    __asm__ volatile ("" ::: "memory");

    static const int sNumberOfThreads = 16;

    StartStopThreadDelegate delegates[sNumberOfThreads];
    for (int i = 0; i < sNumberOfThreads; ++i) {
        delegates[i].mState.store(StartStopThreadDelegate::eState_Down, std::memory_order_relaxed);
        delegates[i].mEnginePtr = &engine;
        delegates[i].mNumberOfAttempts = std::rand() % 17 + 16;
        delegates[i].mIsSuccessful = true;
        delegates[i].mThreadPtr = std::make_unique<std::thread>(StartStopThreadDelegate::TryToStop, &delegates[i], &ctx, &StartStopThreadDelegate::runAttemptsToStartOrStop);
    }

    ctx.mStartFlag.store(true, std::memory_order_release);

    while(true) {
        bool allFinished = true;
        for (int i = 0; i < sNumberOfThreads; ++i) {
            if (StartStopThreadDelegate::eState_Down != delegates[i].mState.load(std::memory_order_relaxed)) {
                allFinished = false;
                break;
            }
        }

        if (allFinished)
            break;

        std::this_thread::yield();
    }

    std::atomic_thread_fence(std::memory_order_acquire);

    for (int i = 0; i < sNumberOfThreads; ++i) {
        if (!delegates[i].mIsSuccessful)
            return false;
    }

    if (ctx.mActiveFlag) {
        UdpResult udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    return true;
}


bool test__udp_sockets_UdpEngine__lifeness_multithread_multi_start_stop() {

    TestUdpEngine engine;

    StartStopThreadDelegate::Context ctx;
    ctx.mStartFlag.store(false, std::memory_order_relaxed);
    ctx.mActiveFlag = false;

    __asm__ volatile ("" ::: "memory");

    static const int sNumberOfThreads = 32;

    StartStopThreadDelegate delegates[sNumberOfThreads];
    for (int i = 0; i < sNumberOfThreads; ++i) {
        delegates[i].mState.store(StartStopThreadDelegate::eState_Down, std::memory_order_relaxed);
        delegates[i].mEnginePtr = &engine;
        delegates[i].mNumberOfAttempts = std::rand() % 17 + 16;
        delegates[i].mIsSuccessful = true;
        delegates[i].mThreadPtr = std::make_unique<std::thread>(StartStopThreadDelegate::TryToStop, &delegates[i], &ctx, &StartStopThreadDelegate::runAttemptsToStartOrStopNoCheck);
    }

    ctx.mStartFlag.store(true, std::memory_order_release);

    while(true) {
        bool allFinished = true;
        for (int i = 0; i < sNumberOfThreads; ++i) {
            if (StartStopThreadDelegate::eState_Down != delegates[i].mState.load(std::memory_order_relaxed)) {
                allFinished = false;
                break;
            }
        }

        if (allFinished)
            break;

        std::this_thread::yield();
    }

    std::atomic_thread_fence(std::memory_order_acquire);

    for (int i = 0; i < sNumberOfThreads; ++i) {
        if (!delegates[i].mIsSuccessful)
            return false;
    }

    if (ctx.mActiveFlag) {
        UdpResult udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    return true;
}


#pragma mark - attachSocket/detachSocket

bool test__udp_sockets_UdpEngine__correctness_singlethread_attach_detach_users() {

    static const int sNumberOfUsers = 4;

    TestUdpUser users[sNumberOfUsers];

    int ports[sNumberOfUsers + 2];
    int startPort = std::rand() % 7000 + 2000;
    for (int i = 0; i < sNumberOfUsers + 2; ++i) {
        ports[i] = startPort + i;
    }

    // everyday i'm shuffleing
    for (int i = sNumberOfUsers + 1; i >= 1; --i) {
        int j = std::rand() % i;
        std::swap(ports[i], ports[j]);
    }

    {
        TestUdpEngine engine;

        UdpResult udpres = engine.startUp();
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        for (int i = 0; i < sNumberOfUsers; ++i) {
            int role = std::rand() % 2;
            udpres = engine.attachSocket(&users[i], role == 0 ? priv::UdpRole::Server : priv::UdpRole::Client, UdpAddress("127.0.0.1", ports[i]));
            CHECK_EQUAL(udpres, eUdpResult_Ok);
        }

        udpres = engine.detachSocket(&users[0]);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.detachSocket(&users[0]);
        CHECK_EQUAL(udpres, eUdpResult_Failed);

        udpres = engine.attachSocket(&users[0], priv::UdpRole::Server, UdpAddress("127.0.0.1", ports[sNumberOfUsers]));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&users[0], priv::UdpRole::Server, UdpAddress("127.0.0.1", ports[sNumberOfUsers + 1]));
        CHECK_EQUAL(udpres, eUdpResult_Already);

        for (int i = 0; i < sNumberOfUsers / 2; ++i) {
            udpres = engine.detachSocket(&users[i]);
            CHECK_EQUAL(udpres, eUdpResult_Ok);
        }

        udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    CHECK_EQUAL(users[0].socketsList().size(), 4);

    for (int i = 1; i < sNumberOfUsers; ++i) {
        CHECK_EQUAL(users[i].socketsList().size(), 2);
    }

    for (int i = 0; i < sNumberOfUsers; ++i) {
        CHECK_EQUAL(users[i].socketsList().back(), -1);
    }

    return true;
}


#pragma mark - dgrams sending/recieving

bool test__udp_sockets_UdpEngine__correctness_singlethread_send_recieve_dgram() {

    static const float sTimoutInMs = 500.0f;

    TestUdpUser server, client;

    UdpDgram dgram({0, 1, 2, 3, 4, 5, 6, 7});
    UdpDgram received;
    {
        TestUdpEngine engine;

        UdpResult udpres = engine.startUp();
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&server, priv::UdpRole::Server, UdpAddress("127.0.0.1", 5051));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&client, priv::UdpRole::Client, UdpAddress("127.0.0.1", 5051));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        bool queres = client.output()->enqueue(dgram.clone());
        CHECK_EQUAL(queres, true);

        std_clock::time_point startTp = std_clock::now();
        while(true) {
            if (server.input()->dequeue(received))
                break;

            std_clock::time_point currentTp = std_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            CHECK_LESS(elapsed.count(), sTimoutInMs);

            std::this_thread::sleep_for(std::chrono::nanoseconds(50));
        }

        UdpDgram tmp;
        bool hadAnother = server.input()->dequeue(tmp);
        CHECK_FALSE(hadAnother);

        udpres = engine.detachSocket(&server);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.detachSocket(&client);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    CHECK_EQUAL(received.size(), dgram.size());

    int cmpres = memcmp(dgram.data(), received.data(), dgram.size());
    CHECK_EQUAL(cmpres, 0);

    return true;
}


bool test__udp_sockets_UdpEngine__correctness_singlethread_send_recieve_answer_dgram() {

    static const float sTimoutInMs = 500.0f;

    TestUdpUser server, client;

    UdpDgram dgram1({0, 1, 2, 3, 4, 5, 6, 7}), dgram2({0, 1, 4, 8, 16, 32, 64, 128});
    UdpDgram received;
    {
        TestUdpEngine engine;

        UdpResult udpres = engine.startUp();
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&server, priv::UdpRole::Server, UdpAddress("127.0.0.1", 5051));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&client, priv::UdpRole::Client, UdpAddress("127.0.0.1", 5051));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        bool queres = client.output()->enqueue(dgram1.clone());
        CHECK_TRUE(queres);

        std_clock::time_point startTp = std_clock::now();
        while(true) {
            if (server.input()->dequeue(received))
                break;

            std_clock::time_point currentTp = std_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            CHECK_LESS(elapsed.count(), sTimoutInMs);

            std::this_thread::sleep_for(std::chrono::nanoseconds(50));
        }

        UdpDgram tmp;
        bool hadAnother = server.input()->dequeue(tmp);
        CHECK_FALSE(hadAnother);

        CHECK_EQUAL(received.size(), dgram1.size());

        int cmpres = memcmp(dgram1.data(), received.data(), dgram1.size());
        CHECK_EQUAL(cmpres, 0);

        queres = server.output()->enqueue(dgram2.clone(received.source()));
        CHECK_TRUE(queres);

        startTp = std_clock::now();
        while(true) {
            if (client.input()->dequeue(received))
                break;

            std_clock::time_point currentTp = std_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            CHECK_LESS(elapsed.count(), sTimoutInMs);

            std::this_thread::sleep_for(std::chrono::nanoseconds(50));
        }

        hadAnother = client.input()->dequeue(tmp);
        CHECK_FALSE(hadAnother);

        CHECK_EQUAL(received.size(), dgram2.size());

        cmpres = memcmp(dgram2.data(), received.data(), dgram2.size());
        CHECK_EQUAL(cmpres, 0);

        udpres = engine.detachSocket(&server);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.detachSocket(&client);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    return true;
}


bool test__udp_sockets_UdpEngine__lifeness_multithread_send_recieve_dgrams() {

    // Two users - server and client; multiple threads; on each thread step,
    // the user is selected, if it is a client, then the thread simply decides
    // to read or write a dgram and performs selected action; if the desired
    // user is a server, then the thread attempts to read a dgram and sends
    // a response to the dgram source if read was successful.

    static const float sTestDurationInMs = 10000.0f;
    static const size_t sNumberOfThreads = 8;

    TestUdpUser server, client;
    SendRecieveThreadDelegate::Context context;
    context.mStartFlag.store(false, std::memory_order_relaxed);
    context.mServerPtr = &server;
    context.mClientPtr = &client;
    {
        TestUdpEngine engine;

        UdpResult udpres = engine.startUp();
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&server, priv::UdpRole::Server, UdpAddress("127.0.0.1", 5051));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.attachSocket(&client, priv::UdpRole::Client, UdpAddress("127.0.0.1", 5051));
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        SendRecieveThreadDelegate delegates[sNumberOfThreads];
        for (size_t i = 0; i < sNumberOfThreads; ++i) {
            delegates[i].mState = SendRecieveThreadDelegate::eState_Down;
            delegates[i].mNumberSentDgrams = 0;
            delegates[i].mNumberRecievedDgrams = 0;
            delegates[i].mNumberRecievedServerDgrams = 0;
            delegates[i].mIsSuccessful = true;

            delegates[i].mThreadPtr =
                std::make_unique<std::thread>( SendRecieveThreadDelegate::DoDelegateWork
                                             , &delegates[i], &context
                                             , &SendRecieveThreadDelegate::doRandomSendsRecievesStep);
        }

        context.mStartFlag.store(true, std::memory_order_release);

        std_clock::time_point startTp = std_clock::now();

        while(true) {
            std_clock::time_point currentTp = std_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            if (elapsed.count() >= sTestDurationInMs)
                break;

            std::this_thread::sleep_for(std::chrono::nanoseconds(50));
        }

        for (size_t i = 0; i < sNumberOfThreads; ++i) {
            SendRecieveThreadDelegate::StateType expected = SendRecieveThreadDelegate::eState_Work;
            delegates[i].mState.compare_exchange_strong(expected, SendRecieveThreadDelegate::eState_Abort, std::memory_order_relaxed);
        }

        while(true) {
            bool allFinished = true;
            for (size_t i = 0; i < sNumberOfThreads; ++i) {
                if (SendRecieveThreadDelegate::eState_Down != delegates[i].mState.load(std::memory_order_relaxed)) {
                    allFinished = false;
                    break;
                }
            }

            if (allFinished)
                break;

            std::this_thread::yield();
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        udpres = engine.detachSocket(&server);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.detachSocket(&client);
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        udpres = engine.tearDown();
        CHECK_EQUAL(udpres, eUdpResult_Ok);

        for (size_t i = 0; i < sNumberOfThreads; ++i) {
            CHECK_GREATER(delegates[i].mNumberSentDgrams, 0);
            CHECK_GREATER(delegates[i].mNumberRecievedDgrams + delegates[i].mNumberRecievedServerDgrams, 0);
        }
    }

    return true;
}
