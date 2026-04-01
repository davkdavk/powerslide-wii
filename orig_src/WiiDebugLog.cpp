#include "WiiDebugLog.h"

#include <cstdarg>
#include <cstdio>

#if defined(WII) || defined(__wii__)
#include <ogc/system.h>
#endif

void WiiDebugLog(const char* fmt, ...)
{
#if defined(WII) || defined(__wii__)
    if(!fmt)
        return;

    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';

    SYS_Report("%s", buffer);

    std::FILE* logFile = std::fopen("sd:/powerslide/debug.log", "a");
    if(!logFile)
        logFile = std::fopen("sd:/debug.log", "a");
    if(!logFile)
        logFile = std::fopen("debug.log", "a");

    if(logFile)
    {
        std::fputs(buffer, logFile);
        std::fclose(logFile);
    }
#else
    (void)fmt;
#endif
}
