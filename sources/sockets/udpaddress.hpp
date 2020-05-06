#ifndef UDP_SOCKETS_UDPADDRESS_HPP_
#define UDP_SOCKETS_UDPADDRESS_HPP_


#include <memory>
#include <string>


namespace udp { ;
namespace sockets { ;


class UdpAddress final {
public:

    static UdpAddress FromNativeData(void* pOpaqueNativeData, size_t szData) noexcept;

    UdpAddress() noexcept;
    UdpAddress(const char* address, int port) noexcept;
   ~UdpAddress() noexcept;

    UdpAddress(const UdpAddress&) = default;
    UdpAddress& operator = (const UdpAddress&) = default;

    bool valid() const noexcept;

    void* nativeData() const noexcept;
    size_t nativeDataSize() const noexcept;

private:

    struct NativeData;

    static NativeData* CreateNativeData(const char* address, int port) noexcept;

    std::shared_ptr<NativeData> _pData;
};


} // namespace sockets
} // namespace udp


#endif//UDP_SOCKETS_UDPADDRESS_HPP_
