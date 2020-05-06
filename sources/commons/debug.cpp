#include "commons/debug.h"

#include <csignal>

#include "commons/logger.hpp"


extern "C" void RaiseHardbreak(const char* file, int line, int counter) {

    char buffer[1024];
    sprintf(buffer, "HARDBREAK at %s [%d:%d]", file, line, counter);
    DumpLogLine(1, buffer);
}


extern "C" void RaiseSoftbreak() {
    
#if defined(DEBUG) || defined(_DEBUG)
    std::raise(SIGTRAP);
#endif
}
