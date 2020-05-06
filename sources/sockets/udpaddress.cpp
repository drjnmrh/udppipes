#include "sockets/udpaddress.hpp"

#include <netdb.h>
#include <unistd.h>

#include <sys/socket.h>

#include "commons/logger.hpp"


#define ERRMACRO_TO_STR(Macro) if (EAI_ ## Macro == result) return # Macro


using namespace udp::sockets;


enum class NativeDataType {
    Addrinfo, Sockaddr
};

struct UdpAddress::NativeData {
    NativeDataType type;
    void*          dataptr{nullptr};
    size_t         size{0};

    ~NativeData() noexcept {

        if (!dataptr)
            return;

        if (NativeDataType::Addrinfo == type) {
            freeaddrinfo((addrinfo*)dataptr);
        } else {
            delete (sockaddr*)dataptr;
        }
    }
};


/*static*/
UdpAddress UdpAddress::FromNativeData(void* pOpaqueNativeData, size_t szData) noexcept {

    sockaddr* pSockAddr = (sockaddr*)pOpaqueNativeData;

    UdpAddress result;
    result._pData = std::make_shared<NativeData>();
    result._pData->type = NativeDataType::Sockaddr;
    result._pData->dataptr = pSockAddr;
    result._pData->size = szData;

    return result;
}


UdpAddress::UdpAddress() noexcept {}


UdpAddress::UdpAddress(const char* address, int port) noexcept
    : _pData(nullptr)
{
    _pData.reset(CreateNativeData(address, port));
}


UdpAddress::~UdpAddress() noexcept {}


bool UdpAddress::valid() const noexcept {

    return !!_pData;
}


void* UdpAddress::nativeData() const noexcept {

    if (!_pData.get())
        return nullptr;

    if (_pData->type == NativeDataType::Addrinfo)
        return ((addrinfo*)_pData->dataptr)->ai_addr;

    return _pData->dataptr;
}


size_t UdpAddress::nativeDataSize() const noexcept {

    if (!_pData.get())
        return 0;
    
    return _pData->size;
}


static const char* ConvertGetAddrInfoErrorToString(int result) {

    ERRMACRO_TO_STR(ADDRFAMILY);
    ERRMACRO_TO_STR(AGAIN);
    ERRMACRO_TO_STR(BADFLAGS);
    ERRMACRO_TO_STR(FAIL);
    ERRMACRO_TO_STR(FAMILY);
    ERRMACRO_TO_STR(MEMORY);
    ERRMACRO_TO_STR(NODATA);
    ERRMACRO_TO_STR(NONAME);
    ERRMACRO_TO_STR(SERVICE);
    ERRMACRO_TO_STR(SOCKTYPE);
    ERRMACRO_TO_STR(SYSTEM);
    ERRMACRO_TO_STR(BADHINTS);
    ERRMACRO_TO_STR(PROTOCOL);
    ERRMACRO_TO_STR(OVERFLOW);
    ERRMACRO_TO_STR(MAX);

    return "UNKNOWN";
}


/*static*/
auto UdpAddress::CreateNativeData(const char* address, int port) noexcept -> NativeData* {

    char portstr[17];
    snprintf(portstr, sizeof(portstr) - 1, "%d", port);
    portstr[16] = '\0';

    addrinfo infoHints = {0};

    infoHints.ai_family = AF_UNSPEC;
    infoHints.ai_socktype = SOCK_DGRAM;
    infoHints.ai_protocol = IPPROTO_UDP;

    addrinfo *pAddrInfo;
    int result = getaddrinfo(address, portstr, &infoHints, &pAddrInfo);
    if (0 != result || !pAddrInfo) {
        LOGE << "ERROR: Failed to get address info (" << address << ":" << port << "; result = " << ConvertGetAddrInfoErrorToString(result) << ")";

        return nullptr;
    }

    NativeData* pData = new NativeData;
    pData->type = NativeDataType::Addrinfo;
    pData->dataptr = pAddrInfo;
    pData->size = pAddrInfo->ai_addrlen;

    return pData;
}
