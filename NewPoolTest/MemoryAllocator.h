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
			NodeStack() : m_pMainHead{}, m_pSubHead{}, m_mainCount{}, m_subCount{} {}

			Node* m_pMainHead;
			Node* m_pSubHead;

			size_t m_mainCount;
			size_t m_subCount;

			bool IsMainEmpty() const { return 0 == m_mainCount && nullptr == m_pMainHead; }
			
			size_t GetTotalCount() const { return m_mainCount + m_subCount; }

		private:
			void SwapHead()
			{
				std::swap(m_pMainHead, m_pSubHead);

				m_mainCount ^= m_subCount; // main ^ sub
				m_subCount ^= m_mainCount; // main ^ sub ^ sub -> main
				m_mainCount ^= m_subCount; // main ^ sub ^ main -> sub
			}
		public:
			void Push(Node* newNode)
			{
				if (nullptr == newNode)
					return;

				if (kNodeCountPerBlock == m_mainCount)
					SwapHead();

				newNode->m_pNextNode = m_pMainHead;
				m_pMainHead = newNode;
				m_mainCount++;

				// 크기가 최대가 되면 하나의 블락으로 두고 다른 블락을 쌓도록 한다.
			}
			
			Node* Pop()
			{
				if (true == IsMainEmpty())
				{
					SwapHead();
					if (true == IsMainEmpty())
						return nullptr;
				}
				
				Node* ret = m_pMainHead;
				
				m_pMainHead = m_pMainHead->m_pNextNode;
				m_mainCount--;
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
		// 할당할 노드가 없을 때 L2의 노드로부터 할당받는다.
		void AcquireBlockFromPool(int poolIdx) 
		{
			m_nodeStack[poolIdx].m_pMainHead = m_pPool[poolIdx]->TryPopBlock();
			m_nodeStack[poolIdx].m_mainCount = m_nodeStack[poolIdx].m_pMainHead->m_blockSize;
		}

		// 데이터가 위치한 포인터만 전달하도록 한다.
		NodeStack m_nodeStack[kPoolCount]; 

		MemoryPool** m_pPool;
    public:	
	};
}

