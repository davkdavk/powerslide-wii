#include "WiiMemoryBudget.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#if defined(WII) || defined(__wii__)
#include <ogc/system.h>
#include "WiiDebugLog.h"
#endif

namespace
{
    static const std::size_t kWiiMemBudgetBytes = 60u * 1024u * 1024u;
    static std::size_t gWiiAllocatedBytes = 0;

    struct WiiAllocHeader
    {
        std::size_t size;
        void* raw;
    };
}

void* Wii_Alloc(std::size_t size, std::size_t alignment)
{
    if(size == 0)
        return NULL;

    if(alignment < sizeof(void*))
        alignment = sizeof(void*);

    const std::size_t overhead = alignment + sizeof(WiiAllocHeader);
    if(gWiiAllocatedBytes + size > kWiiMemBudgetBytes)
    {
#if defined(WII) || defined(__wii__)
        SYS_Report("[WII_ALLOC] budget exceeded req=%u used=%u limit=%u\n",
            static_cast<unsigned int>(size),
            static_cast<unsigned int>(gWiiAllocatedBytes),
            static_cast<unsigned int>(kWiiMemBudgetBytes));
        WiiDebugLog("[WII_ALLOC] budget exceeded req=%u used=%u limit=%u\n",
            static_cast<unsigned int>(size),
            static_cast<unsigned int>(gWiiAllocatedBytes),
            static_cast<unsigned int>(kWiiMemBudgetBytes));
#endif
        return NULL;
    }

    unsigned char* raw = static_cast<unsigned char*>(std::malloc(size + overhead));
    if(!raw)
        return NULL;

    uintptr_t base = reinterpret_cast<uintptr_t>(raw + sizeof(WiiAllocHeader));
    uintptr_t aligned = (base + (alignment - 1)) & ~(static_cast<uintptr_t>(alignment - 1));

    WiiAllocHeader* header = reinterpret_cast<WiiAllocHeader*>(aligned - sizeof(WiiAllocHeader));
    header->size = size;
    header->raw = raw;

    gWiiAllocatedBytes += size;
    return reinterpret_cast<void*>(aligned);
}

void Wii_Free(void* ptr)
{
    if(!ptr)
        return;

    WiiAllocHeader* header = reinterpret_cast<WiiAllocHeader*>(reinterpret_cast<unsigned char*>(ptr) - sizeof(WiiAllocHeader));
    if(header->size <= gWiiAllocatedBytes)
        gWiiAllocatedBytes -= header->size;
    std::free(header->raw);
}

std::size_t Wii_GetAllocatedBytes()
{
    return gWiiAllocatedBytes;
}
