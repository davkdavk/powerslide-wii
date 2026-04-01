#ifndef WII_MEMORY_BUDGET_H
#define WII_MEMORY_BUDGET_H

#include <cstddef>

void* Wii_Alloc(std::size_t size, std::size_t alignment = 32);
void Wii_Free(void* ptr);
std::size_t Wii_GetAllocatedBytes();

#endif
