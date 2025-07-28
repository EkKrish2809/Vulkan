#include "allocator.h"
#include <cstdlib>
// #include <stdlib.h>

void* Allocator::Allocation(size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    return aligned_alloc(size, alignment);
}
void* VKAPI_CALL Allocator::Allocation(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    return static_cast<Allocator*> (pUserData)->Allocation(size, alignment, allocationScope);
}

void* Allocator::Reallocation(void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    return aligned_realloc(pOriginal, size, alignment); // Need linux based implementation
}
void* VKAPI_CALL Allocator::Reallocation(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    return static_cast<Allocator*> (pUserData)->Reallocation(pOriginal, size, alignment, allocationScope);
}

void Allocator::Free(void* pMemory)
{
    aligned_free(pMemory); // Need linux based implementation
}
void VKAPI_CALL Allocator::Free(void* pUserData, void* pMemory)
{
    static_cast<Allocator*> (pUserData)->Free(pMemory);
}