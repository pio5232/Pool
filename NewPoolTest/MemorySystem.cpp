#include "MemorySystem.h"
#include "MemoryAllocator.h"
#include "PageAllocator.h"
#include "MemoryPool.h"
#include "MemoryHeader.h"
#include "Profiler.h"

jh_memory::MemorySystem::MemorySystem()
{
    m_pageAllocator = new PageAllocator();

    for (int i = 0; i < kPoolCount; i++)
    {
        m_poolArr[i] = new MemoryPool(kPoolSizeArr[i]);

        m_poolArr[i]->RegisterPageAllocator(m_pageAllocator);
    }

}

jh_memory::MemorySystem::~MemorySystem()
{    
    for (int i = 0; i < kPoolCount; i++)
    {
        delete m_poolArr[i];

        m_poolArr[i] = nullptr;
    }

    delete m_pageAllocator;
    m_pageAllocator = nullptr;

}

void* jh_memory::MemorySystem::Alloc(size_t reqSize)
{
    MEMORY_POOL_PROFILE_FLAG;
    size_t allocSize = reqSize + sizeof(size_t);
    
    void* pAddr;

    if (allocSize > kMaxAllocSize)
    {
        pAddr = new char[allocSize];
    }
    else
    {
        MemoryAllocator* memoryAllocatorPtr = GetMemoryAllocator();

        pAddr = memoryAllocatorPtr->Alloc(allocSize);
    }

    return MemoryHeader::AttachHeader(static_cast<MemoryHeader*>(pAddr), allocSize);

}

void jh_memory::MemorySystem::Free(void* ptr)
{
    MEMORY_POOL_PROFILE_FLAG;
    MemoryHeader* basePtr = MemoryHeader::DetachHeader(ptr);

    size_t allocSize = basePtr->m_size;

    if (allocSize > kMaxAllocSize)
    {
        delete[] basePtr;
        return;
    }

    MemoryAllocator* memoryAllocatorPtr = GetMemoryAllocator();

    memoryAllocatorPtr->Dealloc(basePtr, allocSize);
}

jh_memory::MemoryAllocator* jh_memory::MemorySystem::GetMemoryAllocator()
{
    MEMORY_POOL_PROFILE_FLAG;
    static thread_local jh_memory::MemoryAllocator memoryAllocator;

    if (true == memoryAllocator.CanRegisterPool())
        memoryAllocator.RegisterPool(m_poolArr);

    return &memoryAllocator;
}
