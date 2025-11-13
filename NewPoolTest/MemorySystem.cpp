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
#ifdef JH_MEM_ALLOC_CHECK_FLAG

    printf("Test Alloc Counter : %lld\n", m_llTestAllocCounter);
    printf("Test Dealloc Counter : %lld\n", m_llTestDeallocCounter);

    LONGLONG poolAllocCountSum = 0;
    LONGLONG poolDeallocCountSum = 0;
    LONGLONG poolTotal = 0;

    for (int i = 0; i < kPoolCount; i++)
    {
        LONGLONG poolAllocCountSum2 = InterlockedExchange64(&m_poolArr[i]->m_llL2AllocedNodeCount, 0);
        LONGLONG poolDeallocCountSum2 = InterlockedExchange64(&m_poolArr[i]->m_llL2DeallocedNodeCount, 0);

        printf("[%d] poolAllocCountSum2 : [%lld]\n", i, poolAllocCountSum2);
        printf("[%d] poolDeallocCountSum2 : [%lld]\n\n", i, poolDeallocCountSum2);

        poolAllocCountSum += poolAllocCountSum2;
        poolDeallocCountSum += poolDeallocCountSum2;

        poolTotal += InterlockedExchange64(&m_poolArr[i]->m_llL2TotalNode, 0);

        delete m_poolArr[i];
        m_poolArr[i] = nullptr;
    }

    delete m_pageAllocator;
    m_pageAllocator = nullptr;

    printf("Test L2 Alloced Sum: %lld\n", poolAllocCountSum);
    printf("Test L2 Dealloced Sum : %lld\n", poolDeallocCountSum);
    printf("Test L2 Created Sum : %lld\n", poolTotal);
#else
    for (int i = 0; i < kPoolCount; i++)
    {
        delete m_poolArr[i];
        m_poolArr[i] = nullptr;
    }

    delete m_pageAllocator;
    m_pageAllocator = nullptr;
#endif
}

void* jh_memory::MemorySystem::Alloc(size_t reqSize)
{
    MEMORY_POOL_PROFILE_FLAG;
    size_t allocSize = reqSize + sizeof(MemoryHeader);

    void* pAddr;
    MemoryAllocator* memoryAllocatorPtr;
    if (allocSize > kMaxAllocSize)
    {
        memoryAllocatorPtr = nullptr;

        pAddr = new char[allocSize];
    }
    else
    {
        memoryAllocatorPtr = GetMemoryAllocator();

        pAddr = memoryAllocatorPtr->Alloc(allocSize);
    }

    return MemoryHeader::AttachHeader(static_cast<MemoryHeader*>(pAddr), memoryAllocatorPtr, allocSize);

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
    
    MemoryAllocator* ownerAllocator = basePtr->m_pOwnerAllocator;
    MemoryAllocator* currentAllocator = GetMemoryAllocator();

    // A 스레드 할당 -> A 스레드 반납의 형태
    if (currentAllocator == ownerAllocator)
    {
        currentAllocator->Dealloc(basePtr, allocSize);
        return;
    }

    // A 스레드 할당 -> B 스레드 반납의 형태
    ownerAllocator->RemoteDealloc(basePtr, allocSize);
}

jh_memory::MemoryAllocator* jh_memory::MemorySystem::GetMemoryAllocator()
{
    MEMORY_POOL_PROFILE_FLAG;
    static thread_local jh_memory::MemoryAllocator memoryAllocator;

    if (true == memoryAllocator.CanRegisterPool())
        memoryAllocator.RegisterPool(m_poolArr);

    return &memoryAllocator;
}
