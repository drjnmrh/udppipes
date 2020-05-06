#include "commons/logger.hpp"

#include <atomic>


using namespace udp;


static std::atomic<uint8_t> sVisibleLevelsMask{0b1111};



/*static*/
void Logger::SetLevelsVisibility(uint8_t mask) noexcept {

    sVisibleLevelsMask.store(mask, std::memory_order_seq_cst);
}



/*static*/
bool Logger::IsLevelVisible(LogLevel level) noexcept {

    uint8_t mask = sVisibleLevelsMask.load(std::memory_order_relaxed);
    LogLevel maskedLevel = static_cast<LogLevel>(mask);

    return (maskedLevel & level);
}



Logger::Logger(LogLevel level) noexcept
    : _bufferEnd(&_buffer[0])
    , _level(level)
{
    _buffer[0] = '\0';
}



void Logger::Append(const char* message) noexcept {

    if (!message)
        return;

    size_t srcPos = 0, dstPos = _bufferEnd - _buffer;
    for (; dstPos < sMaxLogLineLength && message[srcPos] != '\0'; ++srcPos) {
        if ('\n' == message[srcPos]) {
            _bufferEnd = &_buffer[dstPos];
            *_bufferEnd = '\0';

            Dump();

            dstPos = 0;

            continue;
        }

        _buffer[dstPos++] = message[srcPos];
    }

    _bufferEnd = &_buffer[dstPos];
    *_bufferEnd = '\0';
}



void Logger::Dump() noexcept {

    DumpLogLine(static_cast<int>(_level), _buffer);

    _bufferEnd = &_buffer[0];
    *_bufferEnd = '\0';
}
