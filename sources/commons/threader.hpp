#ifndef UDP_COMMONS_THREADER_HPP_
#define UDP_COMMONS_THREADER_HPP_


#include <atomic>
#include <memory>
#include <thread>

#include "commons/macros.h"
#include "commons/types.h"


namespace udp { ;


class Threader final {
    NOCOPY(Threader)
    NOMOVE(Threader)
public:

    using UPtr = std::unique_ptr<Threader>;
    using SPtr = std::shared_ptr<Threader>;

    enum class StepResult {
        Continue, Finished
    };

    using DelegateMethod = StepResult (*) (void*);

    Threader(void* pDelegateData, DelegateMethod delegateMethod) noexcept;
   ~Threader() noexcept;

    UdpResult syncStart(int msTimeout) noexcept;
    UdpResult syncStop (int msTimeout) noexcept;

    bool isRunning() const noexcept;

private:

    struct Delegate {
        void*          mDelegateDataPtr;
        DelegateMethod mDelegateMethod;
    };

    static void DoThreadJob(Threader* pSelf) noexcept;

    CACHELINE(0);

    Delegate _delegate;

    CACHELINE(1);

    std::atomic<int> _state{0};

    CACHELINE(2);
};


} // namespace udp


#endif//UDP_COMMONS_THREADER_HPP_
