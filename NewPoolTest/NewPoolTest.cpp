#define _CRTDBG_MAP_ALLOC
#include <iostream>
#include "Profiler.h"
#include "MemorySystem.h"
#include "CrashDump.h"
#include <crtdbg.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>
#include <random>
#include <conio.h>
#include <thread>
#include <vector>
using namespace std;


struct A
{
	// 512 풀을 사용하도록 크기 조정
	char a[300];
};

const int kThreadCount = 6;
// alloc free Count 지정
const int kInnerLoopCount = 100;// 2000;
const int kOuterLoopCount = 10000;

jh_memory::MemorySystem mSystem;
SRWLOCK g_lock;
std::vector<A*> g_sharedVector;
std::atomic<int> g_completedCount = 0;
std::atomic<bool> g_freeFlag = false;
std::atomic<bool> g_startFlag = false;
const size_t memSize = jh_memory::kPoolSizeArr[(jh_memory::poolTable[sizeof(A)])];


void SameThreadFreeTest();
void CrossThreadFreeTest();
void CrossFree();
void Alloc();


// 중복체크에서 하는 이유
// LONGLONG* duplicationChecker = reinterpret_cast<LONGLONG*>(reinterpret_cast<size_t>(a[j]) + memSize - sizeof(LONGLONG) - sizeof(size_t));

// [size_t] [메모리 할당 위치] ... [] 이므로

// [size_t] [메모리 할당 위치] ... [마지막 8바이트]를 사용하도록 설정.



int main()
{
	InitializeSRWLock(&g_lock);
	g_sharedVector.reserve(kInnerLoopCount * kOuterLoopCount);

	jh_utility::CrashDump crashDump;

	printf("Test Start : [%d] Threads, [%d] Allocs/Loop, [%d] Loops \n\n",kThreadCount, kInnerLoopCount, kOuterLoopCount);

	SameThreadFreeTest();
	//CrossThreadFreeTest();

	mSystem.PrintMemoryUsage();

	printf("------------------- End -------------------\n");

	PRO_SAVE("MemoryCompare.txt");
}

void SameThreadFreeTest()
{

	std::vector<std::thread> threads;
	threads.reserve(kThreadCount);

	for (int i = 0; i < kThreadCount; i++)
	{
		threads.emplace_back([]() {

			A** a = new A * [kInnerLoopCount];

			// -----------------------------------------------
			// 테스트 1: new / delete
			// -----------------------------------------------
			{
				// 테스트 전에 최대한 측정이 동일한 환경에서 이루어지도록 사전작업.
				for (int j = 0; j < kInnerLoopCount; j++)
				{
					a[j] = (new A);
				}
				for (int j = 0; j < kInnerLoopCount; j++)
				{
					delete a[j];
				}

				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

				for (int i = 0; i < kOuterLoopCount; i++)
				{
					for (int j = 0; j < kInnerLoopCount; j++)
					{
						PRO_START("new");
						a[j] = (new A);
						PRO_END("new");
					}

					for (int j = 0; j < kInnerLoopCount; j++)
					{
						PRO_START("delete");
						delete a[j];
						PRO_END("delete");
					}
				}
				auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);

				printf("Thread ID [%u]  |   new/delete : %lld ms\n", GetCurrentThreadId(), diff.count());
			}


			printf("----------------------------------------------------------------------------------\n");
			// -----------------------------------------------
			// 테스트 2: 내 메모리 풀
			// -----------------------------------------------
			{
				// 테스트 전에 최대한 측정이 동일한 환경에서 이루어지도록 사전작업.
				for (int j = 0; j < kInnerLoopCount; j++)
				{
					a[j] = reinterpret_cast<A*>(mSystem.Alloc(sizeof(A)));
				}
				for (int j = 0; j < kInnerLoopCount; j++)
				{
					mSystem.Free(a[j]);
				}

				// 시간 측정
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();


				for (int i = 0; i < kOuterLoopCount; i++)
				{
					// 1. 할당(Alloc)만 연속으로 실행
					// L2에서 L1으로 얻어오는 작업을 실행
					for (int j = 0; j < kInnerLoopCount; j++)
					{
						PRO_START("myPool Alloc");
						a[j] = reinterpret_cast<A*>(mSystem.Alloc(sizeof(A)));
						PRO_END("myPool Alloc");

						LONGLONG* duplicationChecker = reinterpret_cast<LONGLONG*>(reinterpret_cast<size_t>(a[j]) + memSize - sizeof(LONGLONG) - sizeof(size_t));

						if (1 != InterlockedIncrement64(duplicationChecker))
							DebugBreak();

					}

					// 2. 해제(Free)만 연속으로 실행
					// L1에서 L2로 반납
					for (int j = 0; j < kInnerLoopCount; j++)
					{
						LONGLONG* duplicationChecker = reinterpret_cast<LONGLONG*>(reinterpret_cast<size_t>(a[j]) + memSize - sizeof(LONGLONG) - sizeof(size_t));

						if (0 != InterlockedDecrement64(duplicationChecker))
							DebugBreak();

						PRO_START("myPool Free");
						mSystem.Free(a[j]);
						PRO_END("myPool Free");
					}
				}
				auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);

				printf("Thread ID [%u]  |  My Pool      : %lld ms\n", GetCurrentThreadId(), diff.count());
			}

			// 포인터 배열 해제
			delete[] a;
			});
	}
	for (auto& t : threads)
	{

		if (t.joinable())
		{
			t.join();
		}
	}

	while (1)
	{
		printf("Press 'Q' to Exit SameThreadFreeTest() \n");

		if (_kbhit())
		{
			char c = _getch();

			if ('Q' == c || 'q' == c)
				break;
		}

		Sleep(1000);

		mSystem.PrintMemoryUsage();
	}

}
void CrossThreadFreeTest()
{
	std::vector<std::thread> threads;
	threads.reserve(kThreadCount);

	Alloc();
	
	g_freeFlag.wait(false);
	g_startFlag.store(false);

	CrossFree();
}

void Alloc()
{
	std::vector<std::thread> threads;
	threads.reserve(kThreadCount);

	
	for (int i = 0; i < kThreadCount; i++)
	{
		threads.emplace_back([]() {

			g_startFlag.wait(false);

			for (int i = 0; i < kOuterLoopCount; i++)
			{
				for (int j = 0; j < kInnerLoopCount; j++)
				{
					PRO_START("CROSS - Alloc");
					A* pA = reinterpret_cast<A*>(mSystem.Alloc(sizeof(A)));
					PRO_END("CROSS - Alloc");

					LONGLONG* duplicationChecker = reinterpret_cast<LONGLONG*>(reinterpret_cast<size_t>(pA) + memSize - sizeof(LONGLONG) - sizeof(size_t));

					if (1 != InterlockedIncrement64(duplicationChecker))
						DebugBreak();

					AcquireSRWLockExclusive(&g_lock);
					g_sharedVector.push_back(pA);
					ReleaseSRWLockExclusive(&g_lock);
				}
			}

			if (g_completedCount.fetch_add(1) == kThreadCount - 1)
			{
				g_freeFlag.store(true);
				g_freeFlag.notify_all();
			}
			});
	}

	while (1)
	{
		printf("Press 'Q' to Start Alloc() \n");

		if (_kbhit())
		{
			char c = _getch();

			if ('Q' == c || 'q' == c)
				break;
		}

		Sleep(1000);

		mSystem.PrintMemoryUsage();
	}

	g_startFlag.store(true);
	g_startFlag.notify_all();

	while (1)
	{
		printf("Press 'Q' to Exit Alloc() \n");

		if (_kbhit())
		{
			char c = _getch();

			if ('Q' == c || 'q' == c)
				break;
		}

		Sleep(1000);

		mSystem.PrintMemoryUsage();
	}

	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	printf("Alloc() End \n");

}

void CrossFree()
{
	std::vector<std::thread> threads;
	threads.reserve(kThreadCount);

	for (int i = 0; i < kThreadCount; i++)
	{
		threads.emplace_back([]() {
			while (true)
			{
				g_startFlag.wait(false);
				AcquireSRWLockExclusive(&g_lock);

				if (g_sharedVector.size() == 0)
				{
					ReleaseSRWLockExclusive(&g_lock);
					break;
				}

				A* pA = g_sharedVector.back();
				g_sharedVector.pop_back();
				ReleaseSRWLockExclusive(&g_lock);

				LONGLONG* duplicationChecker = reinterpret_cast<LONGLONG*>(reinterpret_cast<size_t>(pA) + memSize - sizeof(LONGLONG) - sizeof(size_t));

				if (0 != InterlockedDecrement64(duplicationChecker))
					DebugBreak();
				PRO_START("CROSS - Free");
				mSystem.Free(pA);
				PRO_END("CROSS - Free");
			}
		});
	}
	while (1)
	{
		printf("Press 'Q' to Start CrossFree() \n");
		if (_kbhit())
		{
			char c = _getch();

			if ('Q' == c || 'q' == c)
				break;
		}

		Sleep(1000);

		mSystem.PrintMemoryUsage();
	}
	g_startFlag.store(true);
	g_startFlag.notify_all();

	while (1)
	{
		printf("Press 'Q' to Exit CrossFree() \n");

		if (_kbhit())
		{
			char c = _getch();

			if ('Q' == c || 'q' == c)
				break;
		}

		Sleep(1000);

		mSystem.PrintMemoryUsage();
	}

	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}
	printf("CrossFree() End \n");

}
