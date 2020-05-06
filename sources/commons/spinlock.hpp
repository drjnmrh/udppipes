#ifndef UDP_COMMONS_SPINLOCK_HPP_
#define UDP_COMMONS_SPINLOCK_HPP_


#include "commons/macros.h"


namespace udp { ;


//! @NOTE(stoned_fox): Since this is a performance critical class I prefer not to use dynamic polymorphism
template < typename PlatformImpl >
class SpinlockBase final {
    NOMOVE(SpinlockBase)
    NOCOPY(SpinlockBase)
public:

    SpinlockBase() noexcept : _impl() { _impl.lock(); }
   ~SpinlockBase() noexcept { _impl.unlock(); }

private:

    PlatformImpl _impl;
};


} // namespace udp


//! @NOTE(stoned_fox): Before considering using a spinlock read this https://blog.postmates.com/why-spinlocks-are-bad-on-ios-b69fc5221058
#if defined(__APPLE__)

#include <libkern/OSAtomic.h>

namespace udp { ;


namespace priv { ;

struct OsxSpinlockImpl {
    OSSpinLock nativeLock;

    OsxSpinlockImpl() noexcept : nativeLock(0) {}

    inline void lock  () noexcept { OSSpinLockLock  (&nativeLock); }
    inline void unlock() noexcept { OSSpinLockUnlock(&nativeLock); }
};

}


using Spinlock = SpinlockBase<priv::OsxSpinlockImpl>;


} // namespace udp

#else

#include <pthread.h>

namespace udp { ;


namespace priv { ;

struct UnixSpinlockImpl {
    pthread_spinlock_t nativeLock;

    UnixSpinlockImpl() noexcept { pthread_spin_init(&nativeLock, 0); }
   ~UnixSpinlockImpl() noexcept { pthread_spin_destroy(&nativeLock); }

    inline void lock  () noexcept { pthread_spin_lock  (&nativeLock); }
    inline void unlock() noexcept { pthread_spin_unlock(&nativeLock); }
};

}


using Spinlock = SpinlockBase<priv::UnixSpinlockImpl>;


}

#endif


#endif//UDP_COMMONS_SPINLOCK_HPP_
