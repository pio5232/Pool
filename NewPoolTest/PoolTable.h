#pragma once


#include <array>

namespace jh_memory
{
	//constexpr size_t kMaxAllocSize = 4096;

	//constexpr std::array<int, kMaxAllocSize + 1> CreatePoolTable()
	//{
 //       std::array<int, kMaxAllocSize + 1> poolTable{};

 //       int targetSize = 0;
 //       int size = 1;
 //       int tableIndex = 0;

 //       for (targetSize = 32; targetSize < 1024; targetSize += 32)
 //       {
 //           while (size <= targetSize)
 //           {
 //               poolTable[size] = tableIndex;

 //               size++;
 //           }

 //           tableIndex++;
 //       }

 //       for (; targetSize < 2048; targetSize += 128)
 //       {
 //           while (size <= targetSize)
 //           {
 //               poolTable[size] = tableIndex;

 //               size++;
 //           }

 //           tableIndex++;
 //       }

 //       for (; targetSize <= 4096; targetSize += 256)
 //       {

 //           while (size <= targetSize)
 //           {
 //               poolTable[size] = tableIndex;

 //               size++;
 //           }

 //           tableIndex++;
 //       }

 //       return poolTable;
	//}
 //   inline constexpr std::array<int, kMaxAllocSize + 1> poolTable = CreatePoolTable(); // Size 요청에 따른 인덱스 탐색을 위한 맵핑 테이블
}