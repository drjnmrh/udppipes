#ifndef UDP_SOCKETS_UDPENGINE_HPP_
#define UDP_SOCKETS_UDPENGINE_HPP_


#include <list>
#include <mutex>
#include <unordered_map>

#include "commons/macros.h"
#include "commons/types.h"

#include "commons/queue.hpp"
#include "commons/threader.hpp"

#include "sockets/udpaddress.hpp"
#include "sockets/udpdgram.hpp"


namespace udp { ;
namespace sockets { ;
namespace priv { ;


using UdpDgramQueue = udp::MpmcBoundedQueue<UdpDgram>;


class IUdpUser {
public:

    virtual ~IUdpUser() noexcept {}

    //! called by the UdpEngine, should not use methods of the UdpEngine.
    virtual void setUp( int socketId
                      , UdpDgramQueue::SPtr pInputQueue
                      , UdpDgramQueue::SPtr pOutputQueue ) noexcept = 0;

    //! might be called from different threads, should not use methods of the UdpEngine.
    virtual void notifyInvalid() noexcept = 0;

};


enum class UdpRole {
    Server, Client
};


class UdpEngine /*final*/ {
    NOCOPY(UdpEngine)
    NOMOVE(UdpEngine)
public:

    static UdpEngine* GetInstancePtr() noexcept;

    UdpResult startUp () noexcept;
    UdpResult tearDown() noexcept;

    UdpResult attachSocket(IUdpUser* pUser, UdpRole role, const UdpAddress& address) noexcept;
    UdpResult detachSocket(IUdpUser* pUser) noexcept;

protected:

    UdpEngine() noexcept;
   ~UdpEngine() noexcept;

private:

    void FindAndFixBadSocketId() noexcept;
    void InvalidateUsersWithLargeSocketId() noexcept;

    struct UserData {
        UdpAddress mAddress;

        UdpDgramQueue::SPtr mInputQueue;
        UdpDgramQueue::SPtr mOutputQueue;

        int mSocketId;
        
        UdpRole mRole;

        std::list<UdpDgram> mLeftovers;
    };

    struct Stats {
        int mNbSent{0};
        int mNbRecieved{0};
        int mNbInputDropped{0};
        int mNbSendFails{0};
        int mNbRecievesFails{0};
    };

    struct NativeData;

    static Threader::StepResult DoEngineStep(void* pOpaqueSelf);

    static void SendUdpUserDgrams   (UserData& udata, Stats* pStats);
    static void RecieveUdpUserDgrams(UserData& udata, Stats* pStats);

    std::mutex                              _usersTableM;
    std::unordered_map<IUdpUser*, UserData> _usersTable;

    NativeData* _pNativeData;

    Threader::UPtr _pThreader;

    Stats _stats;
};


} // namespace priv
} // namespace sockets
} // namespace udp


#endif//UDP_SOCKETS_UDPENGINE_HPP_
