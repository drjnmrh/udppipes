#ifndef UDP_SOCKETS_UDPDGRAM_HPP_
#define UDP_SOCKETS_UDPDGRAM_HPP_


#include <cinttypes>
#include <memory>

#include "commons/macros.h"

#include "sockets/udpaddress.hpp"


namespace udp { ;
namespace sockets { ;


class UdpDgram {
    NOCOPY(UdpDgram)
public:

    UdpDgram() noexcept;
   ~UdpDgram() noexcept;

    UdpDgram(std::initializer_list<uint8_t> data) noexcept;
    UdpDgram(UdpAddress sourceAddress, std::unique_ptr<uint8_t[]> && pData, size_t szData) noexcept;

    UdpDgram(UdpDgram&& another) noexcept;
    UdpDgram& operator = (UdpDgram&& another) noexcept;

    bool valid() const noexcept { return !!_pData; }

    uint8_t* data() noexcept { return _pData; }
    const uint8_t* data() const noexcept { return _pData; }

    size_t size() const noexcept { return _szData; }

    const UdpAddress& source() const noexcept { return _source; }

    UdpDgram clone() const noexcept;
    UdpDgram clone(UdpAddress source) const noexcept;

private:

    static void Swap(UdpDgram& a, UdpDgram& b);

    UdpAddress _source;

    uint8_t* _pData;
    size_t _szData;
};


}
}


#endif//UDP_SOCKETS_UDPDGRAM_HPP_
