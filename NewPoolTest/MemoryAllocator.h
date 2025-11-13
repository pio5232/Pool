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

			bool IsAllEmpty() const { return 0 == m_mainCount && 0 == m_subCount && nullptr == m_pMainHead && nullptr == m_pSubHead; }

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

				// 크기가 최대가 되면 하나의 블락으로 두고 다른 블락을 쌓음.
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

	public:
		MemoryAllocator();
		~MemoryAllocator();

		void* Alloc(size_t allocSize);

		// 같은 스레드에서 할당 - 해제가 이루어졌을 때 노드를 반환.
		void Dealloc(void* ptr, size_t allocSize);

		// 다른 스레드에서 해제가 이루어졌을 때 이 함수를 통해 노드를 반환.
		void RemoteDealloc(void* ptr, size_t allocSize);

		// 노드가 부족해졌을 때 RemoteNodeList에서 Alloc을 통해 mainBlock을 설정한 후 
		// kNodeCountPerBlock보다 적은 개수의 노드 리스트가 남았을 경우
		// 기존 [m_pRemoteNodeListHead]와 다시 연결
		void PushRemoteNodeList(Node* head, Node* tail, int poolIdx);

		bool CanRegisterPool() const { return nullptr == m_pPool; }
		void RegisterPool(MemoryPool** pool) {
			if (true == CanRegisterPool())
				m_pPool = pool;
		}

	private:
		// 할당할 노드가 없을 때 L2의 노드로부터 할당.
		void AcquireBlockFromPool(int poolIdx)
		{
			m_nodeStack[poolIdx].m_pMainHead = m_pPool[poolIdx]->TryPopBlock();
			m_nodeStack[poolIdx].m_mainCount = m_nodeStack[poolIdx].m_pMainHead->m_blockSize;
		}

		Node* AcquireRemoteNodeList(int poolIdx)
		{
			Node* ret = reinterpret_cast<Node*>(InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&m_pRemoteNodeListHead[poolIdx]), nullptr));

			return ret;
		}

		// 원격(다른 스레드)로 반환된 노드를 nodeStack (사용 가능한 노드 리스트)로 연결한다.
		bool MergeRemoteNodeList(int poolIdx);

		// 데이터가 위치한 포인터만 전달.
		NodeStack m_nodeStack[kPoolCount];
		MemoryPool** m_pPool;

		// 원격(다른 스레드)으로 반환된 노드리스트.
		// Alloc 시 main과 sub블락이 비었다면 이것을 main과 sub로 분할.
		alignas(64) Node* m_pRemoteNodeListHead[kPoolCount];
	public:
	};
}

