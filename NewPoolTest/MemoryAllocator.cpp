#include <iostream>
#include "MemoryAllocator.h"
#include "MemoryPool.h"
#include "Profiler.h"
// Pool Table Index

//  + 32
//  Size_32, // ~ 32,       [0]
//  Size_64, // ~ 64,       [1]
//  Size_96, // ~ 96,       [2]
//  Size_128, // ~ 128,     [3]
//  Size_160, // ~ 160,     [4]
//  Size_192, // ~ 192,     [5]
//  Size_224, // ~ 224,     [6]
//  Size_256, // ~ 256,     [7]
//  Size_288, // ~ 288,     [8]
//  Size_320, // ~ 320,     [9]
//  Size_352, // ~ 352,     [10]
//  Size_384, // ~ 384,     [11]
//  Size_416, // ~ 416,     [12]
//  Size_448, // ~ 448,     [13]
//  Size_480, // ~ 480,     [14]
//  Size_512, // ~ 512,     [15]
//  Size_544, // ~ 544,     [16]
//  Size_576, // ~ 576,     [17]
//  Size_608, // ~ 608,     [18]
//  Size_640, // ~ 640,     [19]
//  Size_672, // ~ 672,     [20]
//  Size_704, // ~ 704,     [21]
//  Size_736, // ~ 736,     [22]
//  Size_768, // ~ 768,     [23]
//  Size_800, // ~ 800,     [24]
//  Size_832, // ~ 832,     [25]
//  Size_864, // ~ 864,     [26]
//  Size_896, // ~ 896,     [27]
//  Size_928, // ~ 928,     [28]
//  Size_960, // ~ 960,     [29]
//  Size_992, // ~ 992,     [30]
//  Size_1024, // ~ 1024,   [31]

//  + 128
//  Size_1152, // ~ 1152,   [32]
//  Size_1280, // ~ 1280,   [33]
//  Size_1408, // ~ 1408,   [34]
//  Size_1536, // ~ 1536,   [35]
//  Size_1664, // ~ 1664,   [36]
//  Size_1792, // ~ 1792,   [37]
//  Size_1920, // ~ 1920,   [38]
//  Size_2048, // ~ 2048,   [39]
//
//  + 256
//  Size_2304, // ~ 2304,   [40]
//  Size_2560, // ~ 2560,   [41]
//  Size_2816, // ~ 2816,   [42]
//  Size_3072, // ~ 3072,   [43]
//  Size_3328, // ~ 3328,   [44]
//  Size_3584, // ~ 3584,   [45]
//  Size_3840, // ~ 3840,   [46]
//  Size_4096, // ~ 4096,   [47]

jh_memory::MemoryAllocator::MemoryAllocator() : m_nodeStack{}, m_pPool{}
{

}

jh_memory::MemoryAllocator::~MemoryAllocator()
{
	if (nullptr == m_pPool)
		return;

	for (int i = 0; i < kPoolCount; i++)
	{
		NodeStack& stack = m_nodeStack[i];

		while (1)
		{
			// 스택이 빌 때까지 다음 노드를 꺼냅니다.
			Node* node = stack.Pop();
			
			if (nullptr == node)
				break;
		
			m_pPool[i]->TryPushNode(node);
		}
	}
}

void* jh_memory::MemoryAllocator::Alloc(size_t allocSize)
{
	MEMORY_POOL_PROFILE_FLAG;
	int poolIdx = poolTable[allocSize];

	void* allocedPointer = m_nodeStack[poolIdx].Pop();

	// 할당할 메모리가 없는 경우 L2에서 가져와서 다시 할당한다.
	if (nullptr == allocedPointer)
	{
		AcquireBlockFromPool(poolIdx);
		return m_nodeStack[poolIdx].Pop();
	}
	return allocedPointer;
}



void jh_memory::MemoryAllocator::Dealloc(void* ptr,size_t allocSize)
{
	MEMORY_POOL_PROFILE_FLAG;
	int poolIdx = poolTable[allocSize];

	NodeStack& nodeStack = m_nodeStack[poolIdx];
	// 해제한 메모리 반납한다.
	nodeStack.Push(static_cast<Node*>(ptr));

	// 일정 수량 이상이면 절반을 LEVEL 2에 반납한다.
	
	if (nodeStack.GetTotalCount() == (kNodeCountPerBlock * 2))
	{
		m_pPool[poolIdx]->TryPushBlock(nodeStack.m_pSubHead, kNodeCountPerBlock);
		nodeStack.m_pSubHead = nullptr;
		nodeStack.m_subCount = 0;

	}
}



