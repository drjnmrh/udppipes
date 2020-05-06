#include "sockets/udpdgram.hpp"


using namespace udp::sockets;


UdpDgram::UdpDgram() noexcept
    : _pData(nullptr), _szData(0)
{}


UdpDgram::~UdpDgram() noexcept {

    if (_pData)
        delete[] _pData;
}


UdpDgram::UdpDgram(std::initializer_list<uint8_t> data) noexcept
    : _source()
    , _pData(0), _szData(data.size())
{
    _pData = new uint8_t[data.size()];

    size_t i = 0;
    for (const auto& d : data) {
        _pData[i++] = d;
    }
}


UdpDgram::UdpDgram(UdpAddress sourceAddress, std::unique_ptr<uint8_t[]> && pData, size_t szData) noexcept
    : _source(std::move(sourceAddress))
    , _pData(pData.release()), _szData(szData)
{}


UdpDgram::UdpDgram(UdpDgram&& another) noexcept
    : _source(std::move(another._source))
    , _pData(another._pData), _szData(another._szData)
{
    another._pData = nullptr;
    another._szData = 0;
}


UdpDgram& UdpDgram::operator = (UdpDgram&& another) noexcept {

    if (this != &another) {
        UdpDgram tmp(std::move(another));
        Swap(tmp, *this);
    }

    return *this;
}


UdpDgram UdpDgram::clone() const noexcept {

    if (!_pData) {
        return UdpDgram(_source, nullptr, 0);
    }

    assert(_szData > 0);

    std::unique_ptr<uint8_t[]> pDataCopy = std::make_unique<uint8_t[]>(_szData);
    std::memcpy(pDataCopy.get(), _pData, _szData);

    return UdpDgram(_source, std::move(pDataCopy), _szData);
}


UdpDgram UdpDgram::clone(UdpAddress source) const noexcept {

    if (!_pData) {
        return UdpDgram(source, nullptr, 0);
    }

    assert(_szData > 0);

    std::unique_ptr<uint8_t[]> pDataCopy = std::make_unique<uint8_t[]>(_szData);
    std::memcpy(pDataCopy.get(), _pData, _szData);

    return UdpDgram(source, std::move(pDataCopy), _szData);
}


/*static*/
void UdpDgram::Swap(UdpDgram& a, UdpDgram& b) {

    std::swap(a._source, b._source);
    std::swap(a._pData, b._pData);
    std::swap(a._szData, b._szData);
}
