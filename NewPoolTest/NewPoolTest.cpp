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
#include <vector> // vector 사용

using namespace std;

// 스레드 개수를 CPU 코어 수보다 많게 설정하여 경합을 유도합니다.
const int threadCount = 8;

// L1 캐시(NodeStack)를 확실히 고갈시키기 위해 할당량을 늘립니다.
// kNodeCountPerBlock (1024) 보다 훨씬 많아야 합니다.
const int allocCountPerLoop = 10000;// 2000;

// 테스트 반복 횟수
const int outerLoopCount = 5000;


using namespace std;
struct A
{
	// 1024 풀을 사용하도록 크기 조정
	char a[1000];
};

int main()
{
	// 1. 크래시 덤프 핸들러 초기화
	// (참고: CrashDump 객체는 MemorySystem보다 *먼저* 생성되어야
	// MemorySystem 소멸 시 발생하는 문제를 잡을 수 있습니다.)
	jh_utility::CrashDump crashDump;

	// 2. 메모리 시스템 생성
	jh_memory::MemorySystem mSystem;

	std::vector<std::thread> threads;
	threads.reserve(threadCount);

	printf("Starting Test: %d Threads, %d Allocs/Loop, %d Loops\n\n",
		threadCount, allocCountPerLoop, outerLoopCount);

	for (int i = 0; i < threadCount; i++)
	{
		threads.emplace_back([&mSystem]()
			{
				// 포인터를 저장할 배열 
				A** sys = new A * [allocCountPerLoop];

				// -----------------------------------------------
				// 테스트 1: new / delete
				// -----------------------------------------------
				{
					// 테스트 전에 최대한 측정이 동일한 환경에서 이루어지도록 사전작업.
					for (int j = 0; j < allocCountPerLoop; j++)
					{
						sys[j] = (new A);
					}
					for (int j = 0; j < allocCountPerLoop; j++)
					{
						delete sys[j];
					}

					std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

					for (int i = 0; i < outerLoopCount; i++)
					{
						for (int j = 0; j < allocCountPerLoop; j++)
						{
							PRO_START("new");
							sys[j] = (new A);
							PRO_END("new");
						}

						for (int j = 0; j < allocCountPerLoop; j++)
						{
							PRO_START("delete");
							delete sys[j];
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
					for (int j = 0; j < allocCountPerLoop; j++)
					{
						sys[j] = reinterpret_cast<A*>(mSystem.Alloc(sizeof(A)));
					}
					for (int j = 0; j < allocCountPerLoop; j++)
					{
						mSystem.Free(sys[j]);
					}

					// 시간 측정
					std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

					for (int i = 0; i < outerLoopCount; i++)
					{
						// 1. 할당(Alloc)만 연속으로 실행
						// L2에서 L1으로 얻어오는 작업을 실행
						for (int j = 0; j < allocCountPerLoop; j++)
						{
							PRO_START("myPool Alloc");
							sys[j] = reinterpret_cast<A*>(mSystem.Alloc(sizeof(A)));
							PRO_END("myPool Alloc");
						}

						// 2. 해제(Free)만 연속으로 실행
						// L1에서 L2로 반납
						for (int j = 0; j < allocCountPerLoop; j++)
						{
							PRO_START("myPool Free");
							mSystem.Free(sys[j]);
							PRO_END("myPool Free");
						}
					}
					auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);

					printf("Thread ID [%u]  |  My Pool      : %lld ms\n", GetCurrentThreadId(), diff.count());
				}

				// 포인터 배열 해제
				delete[] sys; 
			});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	printf("------------------- End -------------------\n");

	PRO_SAVE("MemoryCompare.txt");
	 _CrtDumpMemoryLeaks(); // 메모리 풀 사용 시 릭 탐지가 복잡해지므로 주석 처리 권장
}