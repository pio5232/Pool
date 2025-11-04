#pragma once

#include <Windows.h>
#include <vector>


class SRWLockGuard
{
public:
	SRWLockGuard(SRWLOCK* lock) : m_lock{ lock } { AcquireSRWLockExclusive(m_lock); }
	~SRWLockGuard() { ReleaseSRWLockExclusive(m_lock); }

private:
	SRWLOCK* m_lock;
};

namespace jh_memory
{
	class PageAllocator
	{
	public:
		PageAllocator();
		~PageAllocator();

		PageAllocator(const PageAllocator& other) = delete;
		PageAllocator(PageAllocator&& other) = delete;

		PageAllocator& operator=(const PageAllocator& other) = delete;
		PageAllocator& operator=(PageAllocator&& other) = delete;

		void* AllocPage(size_t blockSize);
		void ReleasePage(void* p);
	private:
		SRWLOCK m_lock;

		std::vector<void*> m_pAddressList;

	};
}
