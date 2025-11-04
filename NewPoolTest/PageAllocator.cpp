#include <Windows.h>
#include "PageAllocator.h"
#include "Memory.h"
jh_memory::PageAllocator::PageAllocator()
{
	InitializeSRWLock(&m_lock);

	m_pAddressList.reserve(100);
}

jh_memory::PageAllocator::~PageAllocator()
{
	SRWLockGuard lockGuard(&m_lock);
	for (void* addr : m_pAddressList)
	{
		ReleasePage(addr);
	}

	m_pAddressList.clear();
}

void* jh_memory::PageAllocator::AllocPage(size_t granularitySize)
{	 
	void* p = VirtualAlloc(nullptr, kAllocationGranularity * granularitySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (nullptr == p)
		DebugBreak();

	SRWLockGuard lockGuard(&m_lock);
	m_pAddressList.push_back(p);

	return p;
}

void jh_memory::PageAllocator::ReleasePage(void* p)
{
	if (false == VirtualFree(p, 0, MEM_RELEASE))
	{
		DWORD gle = GetLastError();
		printf("Dealloc Failed. GLE : [%u]\n", gle);
	}
}
