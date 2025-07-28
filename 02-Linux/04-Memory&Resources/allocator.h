#pragma once

#include <vulkan/vulkan.h>


class Allocator 
{
    public:
        Allocator() = default;
        virtual ~Allocator() = default;

        // Operator that allows an instance of this class to be used as VVkAllocationCallbacks structure
        inline operator VkAllocationCallbacks() const 
        {
            VkAllocationCallbacks result = {0};

            result.pUserData = (void*)this;
            result.pfnAllocation = &Allocation;

            // return {
            //     .pUserData = const_cast<Allocator*>(this),
            //     .pfnAllocation = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
            //         return static_cast<Allocator*>(pUserData)->allocate(size, alignment);
            //     },
            //     .pfnReallocation = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
            //         return static_cast<Allocator*>(pUserData)->reallocate(pOriginal, size, alignment);
            //     },
            //     .pfnFree = [](void* pUserData, void* pMemory) {
            //         static_cast<Allocator*>(pUserData)->free(pMemory);
            //     },
            //     .pfnInternalAllocation = nullptr,
            //     .pfnInternalFree = nullptr
            // };
        }

    private:
        // Declare the Allocator callbacks as static member functions
        static void* VKAPI_CALL Allocation(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
        static void* VKAPI_CALL Reallocation(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
        static void VKAPI_CALL Free(void* pUserData, void* pMemory);

        // now declare the nonstatic member functions that will actually perform the allocation
        void* Allocation(size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
        void* Reallocation(void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
        void Free(void* pMemory);
};