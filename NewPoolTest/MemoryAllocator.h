#pragma once

#include "Memory.h"
#include "MemoryPool.h"

namespace jh_memory
{
	class MemoryPool;
	struct Node;
	class MemoryAllocator
	{
		struct NodeStack
		{
			NodeStack() : m_phead{}, m_nodeCount{} {}

			Node* m_phead;
			size_t m_nodeCount;
		
			size_t GetNodeCount() const { return m_nodeCount; }
			bool IsEmpty() const { return m_nodeCount == 0 && nullptr == m_phead; }

			void Push(Node* newNode)
			{
				if (nullptr == newNode)
					return;

				newNode->m_pNextNode = m_phead;
				m_phead = newNode;

				m_nodeCount++;
			}

			Node* Pop()
			{
				if (nullptr == m_phead)
					return nullptr;

				Node* ret = m_phead;
				
				m_phead = m_phead->m_pNextNode;

				m_nodeCount--;

				return ret;

			}
		};


		// ~1024까지 32단위, ~20 48까지 128단위, ~4096까지 256단위
	public:
		MemoryAllocator();
		~MemoryAllocator();
		
		void* Alloc(size_t allocSize);
		void Dealloc(void* ptr, size_t allocSize);

		bool CanRegisterPool() const { return nullptr == m_pPool; }
		void RegisterPool(MemoryPool** pool) {
			if (true == CanRegisterPool())
				m_pPool = pool;
		}

	private:
		void AcquireBlockFromPool(int poolIdx) 
		{
			m_nodeStack[poolIdx].m_phead = m_pPool[poolIdx]->TryPopBlock();
			m_nodeStack[poolIdx].m_nodeCount = m_nodeStack[poolIdx].m_phead->m_blockSize;
		}

		// 데이터가 위치한 포인터만 전달하도록 한다.
		NodeStack m_nodeStack[kPoolCount]; 

		MemoryPool** m_pPool;
    public:	
	};
}

