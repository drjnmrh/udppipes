#include "commons/threader.hpp"

#include <chrono>

#include "commons/logger.hpp"


#define THREADER_STATE_DOWN 0
#define THREADER_STATE_INIT 1
#define THREADER_STATE_WORK 2
#define THREADER_STATE_STOP 3


using namespace udp;


using default_clock = std::chrono::steady_clock;


Threader::Threader(void* pDelegateData, DelegateMethod delegateMethod) noexcept
    : _delegate({.mDelegateDataPtr = pDelegateData, .mDelegateMethod = delegateMethod})
    , _state(THREADER_STATE_DOWN)
{}


Threader::~Threader() noexcept {

    if (_state.load(std::memory_order_relaxed) != THREADER_STATE_DOWN) {
        LOGW << "Destroying active Threader - attempt to stop from d-tor";
        UdpResult res = syncStop(-1);
        if (res != eUdpResult_Ok && res != eUdpResult_Already) {
            LOGE << "Failed to stop Threader - this might cause a crash";
        }
    }
}


UdpResult Threader::syncStart(int msTimeout) noexcept {

    int expectedState = THREADER_STATE_DOWN;
    if (_state.compare_exchange_strong(expectedState, THREADER_STATE_INIT)) {
        std::unique_ptr<std::thread> pThread = std::make_unique<std::thread>(DoThreadJob, this);
        pThread->detach();

        UdpResult res = eUdpResult_Ok;

        default_clock::time_point startTp = default_clock::now();

        while(true) {
            if (_state.load(std::memory_order_relaxed) != THREADER_STATE_INIT)
                break;

            std::this_thread::yield();

            if (msTimeout < 0)
                continue;

            default_clock::time_point currentTp = default_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            if (elapsed.count() >= (float)msTimeout) {
                res = eUdpResult_Timeout;
                break;
            }
        }

        return res;
    }

    return eUdpResult_Already;
}


UdpResult Threader::syncStop(int msTimeout) noexcept {

    default_clock::time_point startTp = default_clock::now();

    int startState = _state.load(std::memory_order_relaxed); // 1
    if (startState == THREADER_STATE_INIT) {
        while(true) {
            if (_state.load(std::memory_order_relaxed) != THREADER_STATE_INIT)
                break;

            std::this_thread::yield();

            if (msTimeout < 0)
                continue;

            default_clock::time_point currentTp = default_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            if (elapsed.count() >= (float)msTimeout) {
                return eUdpResult_Timeout;
            }
        }
    }

    int expectedState = THREADER_STATE_WORK;
    if (_state.compare_exchange_strong(expectedState, THREADER_STATE_STOP)) { // 2
        UdpResult res = eUdpResult_Ok;

        while(true) {
            if (_state.load(std::memory_order_relaxed) != THREADER_STATE_STOP) {
                std::atomic_thread_fence(std::memory_order_acquire);
                break;
            }

            std::this_thread::yield();

            if (msTimeout < 0)
                continue;

            default_clock::time_point currentTp = default_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            if (elapsed.count() >= (float)msTimeout) {
                res = eUdpResult_Timeout;
                break;
            }
        }

        return res;
    } else if (THREADER_STATE_DOWN == expectedState) {
        return eUdpResult_Already;
    } else if (THREADER_STATE_STOP == expectedState) {
        // someone else scheduled stop
        while(true) {
            if (_state.load(std::memory_order_relaxed) != THREADER_STATE_STOP)
                break;

            std::this_thread::yield();

            if (msTimeout < 0)
                continue;

            default_clock::time_point currentTp = default_clock::now();
            std::chrono::duration<float> elapsed = (currentTp - startTp) * 1000.0f;
            if (elapsed.count() >= (float)msTimeout) {
                return eUdpResult_Timeout;
            }
        }

        return eUdpResult_Already;
    } else if (THREADER_STATE_INIT == expectedState) {
        // this is a case when in between 1 and 2 other threads stopped and scheduled start of the thread
        // thus we shouldn't do anything in this thread
        return eUdpResult_Already;
    }

    LOGE << "Unexpected Threader state (" << expectedState << ")";

    return eUdpResult_Failed;
}


bool Threader::isRunning() const noexcept {

    return THREADER_STATE_DOWN != _state.load(std::memory_order_relaxed);
}


/*static*/
void Threader::DoThreadJob(Threader* pSelf) noexcept {

    pSelf->_state.store(THREADER_STATE_WORK, std::memory_order_release);

    while(true) {
        int state = pSelf->_state.load(std::memory_order_relaxed);
        if (THREADER_STATE_STOP == state) {
            break;
        }

        StepResult res = pSelf->_delegate.mDelegateMethod(pSelf->_delegate.mDelegateDataPtr);
        if (StepResult::Finished == res) {
            pSelf->_state.store(THREADER_STATE_STOP, std::memory_order_relaxed);
            break;
        }
    }

    pSelf->_state.store(THREADER_STATE_DOWN, std::memory_order_release);
}
