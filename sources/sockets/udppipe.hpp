#ifndef UDP_SOCKETS_UDPPIPE_HPP_
#define UDP_SOCKETS_UDPPIPE_HPP_


#include <cinttypes>
#include <memory>

#include "commons/macros.h"
#include "commons/types.h"

#include "sockets/udpdgram.hpp"


namespace udp { ;
namespace sockets { ;


class UdpPipe {
public:

    enum class PipeEndType {
        Read, Write
    };

   ~UdpPipe() noexcept;

    UdpResult open(PipeEndType type, const char* address, int16_t port) noexcept;
    UdpResult close() noexcept;

    bool opened() const noexcept;

    UdpResult readDatagram(UdpDgram& outPacket, int32_t msTimeout) noexcept;
    UdpResult writeDatagram(UdpDgram&& inPacket, int32_t msTimeout) noexcept;
};


} // namespace sockets
} // namespace udp


#endif//UDP_SOCKETS_UDPPIPE_HPP_
