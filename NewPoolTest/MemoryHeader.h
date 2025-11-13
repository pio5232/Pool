#pragma once

#include <iostream>

namespace jh_memory
{
	class MemoryPool;
	class MemoryAllocator;

	struct MemoryHeader
	{
		// [MemoryHeader][Data]
		MemoryHeader(class MemoryAllocator* owner, size_t size) : m_pOwnerAllocator{owner}, m_size { size } {}

		static void* AttachHeader(MemoryHeader* header,class MemoryAllocator* owner, size_t size)
		{
			new (header) MemoryHeader(owner, size);

			return reinterpret_cast<void*>(++header);
		}

		static MemoryHeader* DetachHeader(void* ptr)
		{
			return reinterpret_cast<MemoryHeader*>(ptr) - 1;
		}

		class MemoryAllocator* const m_pOwnerAllocator;
		const size_t m_size;
	};
}