#import "Foundation/Foundation.h"

#include "commons/macros.h"


enum LogLevel {
    eLogLevel_Error = 0b0001,
    eLogLevel_Info = 0b0010,
    eLogLevel_Warning = 0b0100,
    eLogLevel_Debug = 0b1000
};


EXTERN void DumpLogLine(int logLevel, char* line) {

    enum LogLevel level = (enum LogLevel)logLevel;
    
    switch(level) {
    case eLogLevel_Error  : NSLog(@"[ERROR] %s", line); break;
    case eLogLevel_Info   : NSLog(@"[INFO] %s", line); break;
    case eLogLevel_Warning: NSLog(@"[WARNING] %s", line); break;
    case eLogLevel_Debug  : NSLog(@"[DEBUG] %s", line); break;
    default: NSLog(@"%s", line);
    }
}
