#include <atomic>
#include <iostream>
#include <list>
#include <mutex>
#include <set>
#include <thread>

#include "commons/macros.h"

#include "commons/threader.hpp"

#include "testapi.hpp"


bool test__udp_Threader__correctness_single_step();
bool test__udp_Threader__correctness_multi_step_multi_stopper();
bool test__udp_Threader__lifeness_multithread_multi_start_stop();


START_TEST_SUIT_DECLARATION(Threader)
    DECLARE_TEST(test__udp_Threader__correctness_single_step)
    DECLARE_TEST(test__udp_Threader__correctness_multi_step_multi_stopper)
    DECLARE_TEST(test__udp_Threader__lifeness_multithread_multi_start_stop)
FINISH_TEST_SUIT_DECLARATION(Threader)


namespace {


struct TestDelegate {

    CACHELINE(0);

    std::atomic<int> mCounter{0};

    CACHELINE(1);

    std::atomic<bool> mCanFinish    {true};
    std::atomic<bool> mCanStartCount{true};

    CACHELINE(2);

    std::mutex                 mCallerThreadsM;
    std::set<std::thread::id>  mCallerThreads;

    CACHELINE(3);

    static udp::Threader::StepResult DoStepOnce(void* pOpaqueData) {

        TestDelegate* pData = (TestDelegate*)pOpaqueData;

        {
            std::scoped_lock<std::mutex> sl(pData->mCallerThreadsM);
            pData->mCallerThreads.insert(std::this_thread::get_id());
        }

        pData->mCounter.fetch_add(1);

        while (true) {
            if (pData->mCanFinish.load(std::memory_order_relaxed))
                break;

            std::this_thread::yield();
        }

        return udp::Threader::StepResult::Finished;
    }

    static udp::Threader::StepResult DoStepMulti(void* pOpaqueData) {

        TestDelegate* pData = (TestDelegate*)pOpaqueData;

        {
            std::scoped_lock<std::mutex> sl(pData->mCallerThreadsM);
            pData->mCallerThreads.insert(std::this_thread::get_id());
        }

        if (pData->mCanStartCount.load(std::memory_order_relaxed))
            pData->mCounter.fetch_add(1);

        return udp::Threader::StepResult::Continue;
    }
};



struct ThreadDelegate {
    enum StateType { eState_Down, eState_Work, eState_Abort };

    struct Context {
        CACHELINE(0);

        std::atomic<bool> mStartFlag;

        CACHELINE(1);
    };

    CACHELINE(0);

    std::atomic<StateType> mState;

    CACHELINE(1);

    std::unique_ptr<std::thread> mThreadPtr;
    udp::Threader* mThreaderPtr;

    int mNumberOfAttempts;
    bool mIsSuccessful;

    CACHELINE(2);

    ~ThreadDelegate() noexcept {

        StateType expected = eState_Work;
        if (mState.compare_exchange_strong(expected, eState_Abort)) {
            while(true) {
                if (eState_Down == mState.load())
                    break;

                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        }
    }

    bool runAttemptsToStartOrStopNoCheck(Context* pCtx) {
        UNUSED(pCtx);

        UdpResult udpres;

        for (; mNumberOfAttempts > 0; --mNumberOfAttempts) {
            int op = std::rand() % 2;
            if (0 == op) {
                udpres = mThreaderPtr->syncStart(-1);
            } else {
                udpres = mThreaderPtr->syncStop(-1);
            }

            CHECK_TRUE(udpres == eUdpResult_Ok || udpres == eUdpResult_Already);

            int delayInNs = std::rand() % 64 + 1;
            std::this_thread::sleep_for(std::chrono::nanoseconds(delayInNs));
        }

        return true;
    }

    static void DoThreadWork(ThreadDelegate* pSelf, Context* pContext, bool (ThreadDelegate::*pMethod)(Context*)) {

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


}



bool test__udp_Threader__correctness_single_step() {

    struct ThreadersLauncher {
        TestDelegate*  mDelegatePtr{nullptr};
        udp::Threader* mThreaderPtr{nullptr};

        std::atomic<int>  mReadyThreadsCounter{0};
        std::atomic<int>  mFinishedThreadsCounter{0};
        std::atomic<bool> mLaunchThreaderFlag{false};

        std::atomic<int> mStartedCounter{0};
        std::atomic<int> mAlreadyCounter{0};
        std::atomic<int> mFailedCounter{0};


        static void LaunchThreader(ThreadersLauncher* pSelf) {

            pSelf->mReadyThreadsCounter.fetch_add(1, std::memory_order_relaxed);

            while(true) {
                if (pSelf->mLaunchThreaderFlag.load(std::memory_order_acquire))
                    break;

                std::this_thread::yield();
            }

            UdpResult res = pSelf->mThreaderPtr->syncStart(-1);
            if (eUdpResult_Ok == res)
                pSelf->mStartedCounter.fetch_add(1, std::memory_order_relaxed);
            else if (eUdpResult_Already == res)
                pSelf->mAlreadyCounter.fetch_add(1, std::memory_order_relaxed);
            else
                pSelf->mFailedCounter.fetch_add(1, std::memory_order_relaxed);

            pSelf->mFinishedThreadsCounter.fetch_add(1, std::memory_order_relaxed);
        }


        void launch(int nbThreads) {

            mReadyThreadsCounter.store(0, std::memory_order_relaxed);
            mFinishedThreadsCounter.store(0, std::memory_order_relaxed);
            mLaunchThreaderFlag.store(false, std::memory_order_relaxed);

            mStartedCounter.store(0, std::memory_order_relaxed);
            mAlreadyCounter.store(0, std::memory_order_relaxed);
            mFailedCounter.store(0, std::memory_order_relaxed);

            mDelegatePtr->mCanFinish.store(false, std::memory_order_relaxed);

            for (int i = 0; i < nbThreads; ++i) {
                std::thread launchThread(&LaunchThreader, this);
                launchThread.detach();
            }

            while(true) {
                if (mReadyThreadsCounter.load(std::memory_order_acquire) >= nbThreads) {
                    mLaunchThreaderFlag.store(true, std::memory_order_release);
                    break;
                }

                std::this_thread::sleep_for(std::chrono::nanoseconds(50));
            }

            while(true) {
                if (mFinishedThreadsCounter.load(std::memory_order_relaxed) >= nbThreads) {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::nanoseconds(5));
            }

            mDelegatePtr->mCanFinish.store(true, std::memory_order_relaxed);

            std::atomic_thread_fence(std::memory_order_acquire);

            while (mThreaderPtr->isRunning()) {
                std::this_thread::yield();
            }
        }
    };

    TestDelegate delegate;

    udp::Threader threader(&delegate, TestDelegate::DoStepOnce);

    __asm__ volatile ("" ::: "memory");

    ThreadersLauncher launcher;
    launcher.mThreaderPtr = &threader;
    launcher.mDelegatePtr = &delegate;

    launcher.launch(32);

    CHECK_EQUAL(launcher.mStartedCounter.load(std::memory_order_relaxed), 1);
    CHECK_EQUAL(launcher.mAlreadyCounter.load(std::memory_order_relaxed), 31);

    CHECK_EQUAL(delegate.mCounter.load(std::memory_order_relaxed), 1);
    CHECK_EQUAL(delegate.mCallerThreads.size(), 1);

    return true;
}


bool test__udp_Threader__correctness_multi_step_multi_stopper() {

    struct ThreadersLauncher {
        TestDelegate*  mDelegatePtr{nullptr};
        udp::Threader* mThreaderPtr{nullptr};

        std::atomic<int>  mReadyThreadsCounter{0};
        std::atomic<int>  mFinishedThreadsCounter{0};
        std::atomic<int>  mWaitingThreadsCounter{0};
        std::atomic<bool> mLaunchThreaderFlag{false};
        std::atomic<bool> mStopThreaderFlag{false};

        std::atomic<int> mOkStartedCounter{0};
        std::atomic<int> mAlreadyStartedCounter{0};
        std::atomic<int> mFailedStartedCounter{0};

        std::atomic<int> mOkStoppedCounter{0};
        std::atomic<int> mAlreadyStoppedCounter{0};
        std::atomic<int> mFailedStoppedCounter{0};

        static void LaunchThreader(ThreadersLauncher* pSelf) {

            UdpResult res;

            int nsecsToWaitBeforeStart = std::rand() % 50 + 1;
            int nsecsToWaitBeforeStop  = std::rand() % 50 + 1;

            pSelf->mReadyThreadsCounter.fetch_add(1, std::memory_order_relaxed);

            while(true) {
                if (pSelf->mLaunchThreaderFlag.load(std::memory_order_acquire))
                    break;

                std::this_thread::yield();
            }

            std::this_thread::sleep_for(std::chrono::nanoseconds(nsecsToWaitBeforeStart));

            res = pSelf->mThreaderPtr->syncStart(-1);
            if (eUdpResult_Ok == res)
                pSelf->mOkStartedCounter.fetch_add(1, std::memory_order_relaxed);
            else if (eUdpResult_Already == res)
                pSelf->mAlreadyStartedCounter.fetch_add(1, std::memory_order_relaxed);
            else
                pSelf->mFailedStartedCounter.fetch_add(1, std::memory_order_relaxed);

            pSelf->mWaitingThreadsCounter.fetch_add(1, std::memory_order_release);

            while(true) {
                if (pSelf->mStopThreaderFlag.load(std::memory_order_acquire))
                    break;

                std::this_thread::yield();
            }

            std::this_thread::sleep_for(std::chrono::nanoseconds(nsecsToWaitBeforeStop));

            res = pSelf->mThreaderPtr->syncStop(-1);
            if (eUdpResult_Ok == res)
                pSelf->mOkStoppedCounter.fetch_add(1, std::memory_order_relaxed);
            else if (eUdpResult_Already == res)
                pSelf->mAlreadyStoppedCounter.fetch_add(1, std::memory_order_relaxed);
            else
                pSelf->mFailedStoppedCounter.fetch_add(1, std::memory_order_relaxed);

            pSelf->mFinishedThreadsCounter.fetch_add(1, std::memory_order_relaxed);
        }


        void launch(int nbThreads, int valueToWait) {

            mReadyThreadsCounter   .store(0, std::memory_order_relaxed);
            mFinishedThreadsCounter.store(0, std::memory_order_relaxed);

            mLaunchThreaderFlag.store(false, std::memory_order_relaxed);
            mStopThreaderFlag  .store(false, std::memory_order_relaxed);

            mWaitingThreadsCounter.store(0, std::memory_order_relaxed);

            mOkStartedCounter     .store(0, std::memory_order_relaxed);
            mAlreadyStartedCounter.store(0, std::memory_order_relaxed);
            mFailedStartedCounter .store(0, std::memory_order_relaxed);

            mOkStoppedCounter     .store(0, std::memory_order_relaxed);
            mAlreadyStoppedCounter.store(0, std::memory_order_relaxed);
            mFailedStoppedCounter .store(0, std::memory_order_relaxed);

            mDelegatePtr->mCanStartCount.store(false, std::memory_order_relaxed);


            for (int i = 0; i < nbThreads; ++i) {
                std::thread launchThread(&LaunchThreader, this);
                launchThread.detach();
            }

            while(true) {
                if (mReadyThreadsCounter.load(std::memory_order_acquire) >= nbThreads) {
                    mLaunchThreaderFlag.store(true, std::memory_order_release);
                    break;
                }

                std::this_thread::sleep_for(std::chrono::nanoseconds(50));
            }

            while(true) {
                if (mWaitingThreadsCounter.load(std::memory_order_acquire) >= nbThreads) {
                    mDelegatePtr->mCanStartCount.store(true);
                    break;
                }

                std::this_thread::sleep_for(std::chrono::nanoseconds(5));
            }

            while(true) {
                if (mDelegatePtr->mCounter.load(std::memory_order_relaxed) >= valueToWait) {
                    mStopThreaderFlag.store(true, std::memory_order_release);
                    break;
                }
            }

            while(true) {
                if (mFinishedThreadsCounter.load(std::memory_order_relaxed) >= nbThreads) {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::nanoseconds(5));
            }

            std::atomic_thread_fence(std::memory_order_acquire);

            while (mThreaderPtr->isRunning()) {
                std::this_thread::yield();
            }
        }
    };

    TestDelegate delegate;

    udp::Threader threader(&delegate, TestDelegate::DoStepMulti);

    __asm__ volatile ("" ::: "memory");

    ThreadersLauncher launcher;
    launcher.mThreaderPtr = &threader;
    launcher.mDelegatePtr = &delegate;

    launcher.launch(32, 1000);

    CHECK_EQUAL(launcher.mOkStartedCounter.load(std::memory_order_relaxed), 1);
    CHECK_EQUAL(launcher.mAlreadyStartedCounter.load(std::memory_order_relaxed), 31);

    CHECK_EQUAL(launcher.mOkStoppedCounter.load(std::memory_order_relaxed), 1);
    CHECK_EQUAL(launcher.mAlreadyStoppedCounter.load(std::memory_order_relaxed), 31);

    CHECK_GREATER(delegate.mCounter.load(std::memory_order_relaxed), 999);
    CHECK_EQUAL(delegate.mCallerThreads.size(), 1);

    return true;
}


bool test__udp_Threader__lifeness_multithread_multi_start_stop() {

    TestDelegate delegate;

    udp::Threader threader(&delegate, TestDelegate::DoStepMulti);

    ThreadDelegate::Context ctx;
    ctx.mStartFlag.store(false, std::memory_order_relaxed);

    __asm__ volatile ("" ::: "memory");

    static const int sNumberOfThreads = 32;

    ThreadDelegate delegates[sNumberOfThreads];
    for (int i = 0; i < sNumberOfThreads; ++i) {
        delegates[i].mState.store(ThreadDelegate::eState_Down, std::memory_order_relaxed);
        delegates[i].mThreaderPtr = &threader;
        delegates[i].mNumberOfAttempts = std::rand() % 17 + 16;
        delegates[i].mIsSuccessful = true;
        delegates[i].mThreadPtr =
            std::make_unique<std::thread>(ThreadDelegate::DoThreadWork, &delegates[i], &ctx, &ThreadDelegate::runAttemptsToStartOrStopNoCheck);
    }

    ctx.mStartFlag.store(true, std::memory_order_release);

    while(true) {
        bool allFinished = true;
        for (int i = 0; i < sNumberOfThreads; ++i) {
            if (ThreadDelegate::eState_Down != delegates[i].mState.load(std::memory_order_relaxed)) {
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

    if (threader.isRunning()) {
        UdpResult udpres = threader.syncStop(-1);
        CHECK_EQUAL(udpres, eUdpResult_Ok);
    }

    return true;
}
