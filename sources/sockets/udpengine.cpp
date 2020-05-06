#include "sockets/udpengine.hpp"

#include <assert.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <list>

#include "commons/logger.hpp"
#include "commons/utils.hpp"


#define DGRAM_MAXLINE 1024
#define DGRAM_QUEUE_SIZE 512


using namespace udp;
using namespace sockets::priv;


struct UdpEngine::NativeData {
    fd_set mAllSockets; /// @NOTE(stoned_fox): since this is a reflection of the udp users table, this
                        ///                data is guarded by the same mutex as the table.
    int mMaxSocketId;
};


static UdpEngine* sUdpEngineInstancePtr = nullptr;


class UdpEngineInstance : public UdpEngine {
public:

    UdpEngineInstance() noexcept = default;

   ~UdpEngineInstance() noexcept {

        if (__builtin_expect(sUdpEngineInstancePtr == this, 1)) {
            sUdpEngineInstancePtr = nullptr;
        }
    }

};


static void InstantiateUdpEngine() {

    static UdpEngineInstance sUdpEngineInstance;
    sUdpEngineInstancePtr = &sUdpEngineInstance;
}


/*static*/
UdpEngine* UdpEngine::GetInstancePtr() noexcept {

    static intptr_t sInitFlag = 0;
    udp::dispatch_once(&sInitFlag, &InstantiateUdpEngine);

    if (!sUdpEngineInstancePtr) {
        // this can happen only if some code tries to process some pipes after
        // the app started closing; e.g. when some static udp pipe dies after
        // the udp engine; in this case udp pipe should realize futility of its
        // actions and just die

        assert(sUdpEngineInstancePtr != nullptr); // keep this false assert to get a hardbreak in a debug build

        return nullptr;
    }

    return sUdpEngineInstancePtr;
}


UdpResult UdpEngine::startUp() noexcept {

    return _pThreader->syncStart(-1);
}


UdpResult UdpEngine::tearDown() noexcept {

    return _pThreader->syncStop(-1);
}


UdpResult UdpEngine::attachSocket(IUdpUser* pUser, UdpRole role, const UdpAddress& address) noexcept {

    UserData udata;
    udata.mAddress = address;
    udata.mInputQueue  = std::make_shared<UdpDgramQueue>(DGRAM_QUEUE_SIZE);
    udata.mOutputQueue = std::make_shared<UdpDgramQueue>(DGRAM_QUEUE_SIZE);
    udata.mRole = role;

    if ((udata.mSocketId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        LOGE << "Failed to create socket";
        return eUdpResult_Failed;
    }

    if (UdpRole::Server == role) {
        int result = bind(udata.mSocketId, (sockaddr*)address.nativeData(), address.nativeDataSize());
        if (0 != result) {
            LOGE << "Failed to bind address to a socket (errno == " << errno << ")";

            close(udata.mSocketId);

            return eUdpResult_Failed;
        }
    }

    TRY_LOCKED(_usersTable) {
        auto insres = _usersTable.insert(std::make_pair(pUser, udata));
        if (!insres.second) {
            LOGE << "Trying to attach already attached user";

            close(udata.mSocketId);

            return eUdpResult_Already;
        }

        FD_SET(udata.mSocketId, &_pNativeData->mAllSockets);

        if (udata.mSocketId + 1 > _pNativeData->mMaxSocketId)
            _pNativeData->mMaxSocketId = udata.mSocketId + 1;
    } UNLOCK;

    pUser->setUp(udata.mSocketId, udata.mInputQueue, udata.mOutputQueue);

    return eUdpResult_Ok;
}


UdpResult UdpEngine::detachSocket(IUdpUser* pUser) noexcept {

    TRY_LOCKED(_usersTable) {
        auto foundIt = _usersTable.find(pUser);
        if (_usersTable.end() == foundIt) {
            LOGE << "Trying to detach not attached user";
            return eUdpResult_Failed;
        }

        pUser->notifyInvalid();
        
        FD_CLR(foundIt->second.mSocketId, &_pNativeData->mAllSockets);

        close(foundIt->second.mSocketId);

        _usersTable.erase(foundIt);
    } UNLOCK;

    return eUdpResult_Ok;
}


UdpEngine::UdpEngine() noexcept
    : _pNativeData(new NativeData)
{
    FD_ZERO(&_pNativeData->mAllSockets);

    _pNativeData->mMaxSocketId = 0;

    _pThreader = std::make_unique<Threader>(this, &DoEngineStep);
}


UdpEngine::~UdpEngine() noexcept {

    TRY_LOCKED(_usersTable) {
        if (_usersTable.size() > 0) {
            LOGW << "Destroying Udp Engine while still having active users";
            for (auto& p : _usersTable) {
                p.first->notifyInvalid();

                close(p.second.mSocketId);
            }

            _usersTable.clear();

            FD_ZERO(&_pNativeData->mAllSockets);
        }
    } UNLOCK;

    _pThreader->syncStop(-1);

    delete _pNativeData;
}


void UdpEngine::FindAndFixBadSocketId() noexcept {

    std::list<IUdpUser*> badIds;

    for (auto& p : _usersTable) {
        fd_set toTry;
        FD_ZERO(&toTry);
        FD_SET(p.second.mSocketId, &toTry);

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int selectRes = select(p.second.mSocketId + 1, &toTry, 0, 0, &tv);
        if (EBADF == selectRes) {
            p.first->notifyInvalid();
            badIds.push_back(p.first);
        }
    }

    for (auto pUser : badIds) {
        _usersTable.erase(pUser);
    }
}


void UdpEngine::InvalidateUsersWithLargeSocketId() noexcept {

    std::list<IUdpUser*> badIds;

    for (auto& p : _usersTable) {
        if (p.second.mSocketId >= FD_SETSIZE) {
            p.first->notifyInvalid();
            badIds.push_back(p.first);
        }
    }

    for (auto pUser : badIds) {
        _usersTable.erase(pUser);
    }
}


/*static*/
Threader::StepResult UdpEngine::DoEngineStep(void *pOpaqueSelf) {

    UdpEngine* pSelf = (UdpEngine*)pOpaqueSelf;

    fd_set toRead, toWrite, withErrors;
    FD_ZERO(&withErrors);

    TRY_LOCKED(pSelf->_usersTable) {
        if (pSelf->_pNativeData->mMaxSocketId <= 0)
            return Threader::StepResult::Continue;

        if (pSelf->_usersTable.size() == 0)
            return Threader::StepResult::Continue;

        FD_COPY(&pSelf->_pNativeData->mAllSockets, &toRead);
        FD_COPY(&pSelf->_pNativeData->mAllSockets, &toWrite);

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int selectRes = select(pSelf->_pNativeData->mMaxSocketId, &toRead, &toWrite, &withErrors, &tv);
        if (EAGAIN == selectRes || EINTR == selectRes) {
            // just continue and try on the next step
            return Threader::StepResult::Continue;
        } else if (EBADF == selectRes) {
            pSelf->FindAndFixBadSocketId();
            return Threader::StepResult::Continue;
        } else if (EINVAL == selectRes) {
            if (pSelf->_pNativeData->mMaxSocketId >= FD_SETSIZE) {
                // too many descriptors! lets throw some away
                pSelf->InvalidateUsersWithLargeSocketId();
                return Threader::StepResult::Continue;
            }

            // according to the man page, this is a case, when timeval is wrong.
            // this is definitely a hardbreak...
            HARDBREAK;
        } else if (0 == selectRes) {
            // timeout case - try on the next step
            return Threader::StepResult::Continue;
        } else if (selectRes < 0) {
            // welp, this is strange - let's break the fuck out of here
            HARDBREAK;
        }

        assert(selectRes > 0);

        for (auto& it : pSelf->_usersTable) {
            if (FD_ISSET(it.second.mSocketId, &toWrite) != 0) {
                SendUdpUserDgrams(it.second, &pSelf->_stats);
            }

            if (FD_ISSET(it.second.mSocketId, &toRead) != 0) {
                RecieveUdpUserDgrams(it.second, &pSelf->_stats);
            }
        }
    } UNLOCK;

    return Threader::StepResult::Continue;
}


/*static*/
void UdpEngine::SendUdpUserDgrams(UserData& udata, Stats* pStats) {

    const UdpAddress* pAddress = nullptr;
    if (udata.mRole == UdpRole::Client) {
        pAddress = &udata.mAddress;
    }

    UdpDgram dgram;
    if (udata.mLeftovers.size() > 0) {
        dgram = std::move(udata.mLeftovers.front());
        udata.mLeftovers.pop_front();
    } else {
        udata.mOutputQueue->dequeue(dgram);
    }

    if (dgram.valid()) {
        if (!pAddress) {
            pAddress = &(dgram.source());
        }
        sockaddr* pAddrInfo = (sockaddr*)pAddress->nativeData();
        size_t szSent = sendto(udata.mSocketId, dgram.data(), dgram.size(), 0, pAddrInfo, pAddress->nativeDataSize());

        if (szSent <= 0) {
            LOGW << "failed to send data!";
            if (pStats) {
                pStats->mNbSendFails += 1;
            }

            udata.mLeftovers.push_front(std::move(dgram));
        } else {
            if (pStats) {
                pStats->mNbSent += 1;
            }


        }
    }
}


/*static*/
void UdpEngine::RecieveUdpUserDgrams(UserData& udata, Stats* pStats) {

    static uint8_t sBuffer[DGRAM_MAXLINE];
    int nbReadBytes;
    socklen_t szAddress;

    std::unique_ptr<sockaddr_in> pSrcAddress = std::make_unique<sockaddr_in>();

    nbReadBytes = recvfrom(udata.mSocketId, (char*)sBuffer, DGRAM_MAXLINE, MSG_WAITALL, (struct sockaddr*)pSrcAddress.get(), &szAddress);

    if (nbReadBytes <= 0) {
        LOGW << "failed to recieve data!";
        if (pStats) {
            pStats->mNbRecievesFails += 1;
        }

        return;
    }

    std::unique_ptr<uint8_t[]> pData = std::make_unique<uint8_t[]>(nbReadBytes);
    memcpy(pData.get(), sBuffer, nbReadBytes);

    if (pStats) {
        pStats->mNbRecieved += 1;
    }

    UdpDgram dgram(UdpAddress::FromNativeData(pSrcAddress.release(), szAddress), std::move(pData), nbReadBytes);
    if (!udata.mInputQueue->enqueue(std::move(dgram))) {
        LOGW << "Failed to enqueue recieved dgram - dropped";
        if (pStats) {
            pStats->mNbInputDropped += 1;
        }
    }
}
