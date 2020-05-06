#ifndef UDP_COMMONS_LOGGER_HPP_
#define UDP_COMMONS_LOGGER_HPP_


#include <cinttypes>
#include <sstream>

#include "commons/macros.h"


EXTERN void DumpLogLine(int logLevel, char* line); /// @NOTE: Must be thread-safe.


namespace udp { ;


enum LogLevel {
    eLogLevel_Error = 0b0001, eLogLevel_Info = 0b0010, eLogLevel_Warning = 0b0100, eLogLevel_Debug = 0b1000
};



//! @NOTE: Not thread-safe - use local objects.
class Logger {
    NOCOPY(Logger)
    NOMOVE(Logger)
public:

    static void SetLevelsVisibility(uint8_t mask)   noexcept;
    static bool IsLevelVisible     (LogLevel level) noexcept;

    Logger(LogLevel level) noexcept;
   ~Logger() noexcept { Dump(); }

    void Append(const char* message) noexcept;

    void Dump() noexcept;

    template <typename T>
    Logger& operator << (T v) noexcept {

        TRY {
            Append((std::stringstream() << v).str().c_str());
        } CATCHALL {
            std::abort();
        }

        return *this;
    }

private:

    static constexpr size_t sMaxLogLineLength = 128 - sizeof(char*) - sizeof(LogLevel) - 1;

    char _buffer[sMaxLogLineLength + 1];
    char* _bufferEnd;
    LogLevel _level;

};


}


#define LOG(Level) if (udp::Logger::IsLevelVisible(udp::eLogLevel_ ## Level)) udp::Logger(udp::eLogLevel_ ## Level)

#define LOGE LOG(Error)
#define LOGI LOG(Info)
#define LOGW LOG(Warning)
#define LOGD LOG(Debug)


#endif//UDP_COMMONS_LOGGER_HPP_
