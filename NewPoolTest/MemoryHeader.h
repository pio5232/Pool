#pragma once

#include "PoolTable.h"

#include <iostream>

namespace jh_memory
{
	class MemoryPool;

	struct MemoryHeader
	{
		// [MemoryHeader][Data]
		MemoryHeader(size_t size) : m_size{ size } {}

		static void* AttachHeader(MemoryHeader* header, size_t size)
		{
			// placement new

			new (header) MemoryHeader(size);

			return reinterpret_cast<void*>(++header);
		}

		static MemoryHeader* DetachHeader(void* ptr)
		{
			return reinterpret_cast<MemoryHeader*>(ptr) - 1;
		}

		const size_t m_size;
	};
}