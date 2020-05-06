#include "commons/utils.hpp"

#include <stdatomic.h>
#include <pthread.h>


#define FLAG_DONE 2
#define FLAG_BUSY 1
#define FLAG_FREE 0



void udp::dispatch_once(intptr_t* flag, udp::InvocationBlock block) {

    intptr_t flagValue = atomic_load_explicit((_Atomic intptr_t*)flag, memory_order_acquire);
    if (__builtin_expect(flagValue == FLAG_DONE, 1)) {
        return;
    }

    intptr_t expected = FLAG_FREE;

    if (atomic_compare_exchange_strong((_Atomic intptr_t*)flag, &expected, FLAG_BUSY)) {
        block();
        atomic_store_explicit((_Atomic intptr_t*)flag, FLAG_DONE, memory_order_release);
    } else {
        while (atomic_load_explicit((_Atomic intptr_t*)flag, memory_order_relaxed) != FLAG_DONE) {
#if defined(_MSC_VER)
            Sleep(0);
#else
            pthread_yield_np();
#endif
        }
        atomic_thread_fence(memory_order_acquire);
    }

    __builtin_assume(*flag == FLAG_DONE);
}
