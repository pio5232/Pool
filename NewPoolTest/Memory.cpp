#include "Memory.h"

//constexpr std::array<int, jh_memory::kMaxAllocSize + 1> jh_memory::CreatePoolTable()
//{
//    std::array<int, kMaxAllocSize + 1> poolTable{};
//
//    int targetSize = 0;
//    int size = 1;
//    int tableIndex = 0;
//
//    for (targetSize = 64; targetSize <= kMaxAllocSize; targetSize *= 2)
//    {
//        while (size <= targetSize)
//        {
//            poolTable[size] = tableIndex;
//            size++;
//        }
//
//        tableIndex++;
//    }
//    return poolTable;
//}
    //for (targetSize = 32; targetSize < 1024; targetSize += 32)
    //{
    //    while (size <= targetSize)
    //    {
    //        poolTable[size] = tableIndex;

    //        size++;
    //    }

    //    tableIndex++;
    //}

    //for (; targetSize < 2048; targetSize += 128)
    //{
    //    while (size <= targetSize)
    //    {
    //        poolTable[size] = tableIndex;

    //        size++;
    //    }

    //    tableIndex++;
    //}

    //for (; targetSize <= 4096; targetSize += 256)
    //{

    //    while (size <= targetSize)
    //    {
    //        poolTable[size] = tableIndex;

    //        size++;
    //    }

    //    tableIndex++;
    //}
//
//    return poolTable;
//}
