// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ConnectThreadPoolThread.h"

FConnectThreadPoolThread::FConnectThreadPoolThread()
	:FRunnable()
	,TimeToDie(false)
	, SleepTimeSpan(0)
	, bCleanup(true)
{
	ConnectThreadPool.Reset(101);
}

FConnectThreadPoolThread::FConnectThreadPoolThread(int32 InMaxBacklog)
	: FRunnable()
	,TimeToDie(false)
	, SleepTimeSpan(0)
	, bCleanup(true)
{
	ConnectThreadPool.Reset(InMaxBacklog + 1);
}

FConnectThreadPoolThread::~FConnectThreadPoolThread()
{
	if (LifecycleStep.GetValue() == 2)
	{
		UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread destruct: cannot exit safe"));
	}

	// cleanup thread
	if (nullptr != Thread)
	{
		delete Thread;
		Thread = nullptr;
	}
	// cleanup events
	// null events here

	// cleanup init ptr
	// when init false, that won't call Exit()

	SafeCleanupPool();
	SafeCleanupQueue();
}

bool FConnectThreadPoolThread::Init()
{
	LifecycleStep.Set(1);
	// if init success, return true here

	SafeCleanupPool();
	return true;
}

uint32 FConnectThreadPoolThread::Run()
{
	LifecycleStep.Set(2);
	//FPlatformMisc::MemoryBarrier();

	// [Warnning] Mustn't use bStopped here!
	while(!TimeToDie)
	{
		FPlatformProcess::Sleep(SleepTimeSpan);

		if (bCleanup) {
			// remove disconnected client
			for (int32 i = ConnectThreadPool.Num() - 1; i >= 0; --i)
			{
				if (nullptr != ConnectThreadPool[i])
				{
					if (!ConnectThreadPool[i]->IsSocketConnection())
					{
						// soft kill:
						// 1. stop to let the thread exit by itself
						// 2. add the thread ptr to destruct queue
						// 3. wait & clean the queue outside
						ConnectThreadPool[i]->Stop();
						// enqueue to the wait to clean queue .  thread-safe
						DestructQueue.Enqueue(ConnectThreadPool[i]);
						// the ptr had been hold in DestructQueue, don't need delete
						//ConnectThreadPool[i] = nullptr;
						// O(1): remove the last hole element and swap with the last element. Don't shrink array!
						// the rank will not be out of order
						ConnectThreadPool.RemoveAtSwap(i, 1, false);
					}
				}
				else {
					// nullptr
					ConnectThreadPool.RemoveAtSwap(i, 1, false);
				}
			}

			// soft clean the queue
			FConnectThread* t_thread = nullptr;
			if (DestructQueue.Peek(t_thread))
			{
				if (nullptr == t_thread || t_thread->IsExited())
				{
					if (DestructQueue.Dequeue(t_thread))
					{
						if (nullptr != t_thread)
						{
							// t_thread had soft exit
							delete t_thread;
							t_thread = nullptr;

							//t_thread->KillThread();
						}
					}
				}

			}
		}
		
	}

	// ExitCode:0 means no error
	return 0;
}

void FConnectThreadPoolThread::Stop()
{
	if (!bStopped) {
		TimeToDie = true;
		// call father function
		//FRunnable::Stop(); // father function == {}

		// because pthread->kill will call stop
		// you cannot call pthread->kill here

		bStopped = true;
	}
}

void FConnectThreadPoolThread::Exit()
{
	LifecycleStep.Set(3);
	// cleanup Run() ptr;
	SafeCleanupQueue();
	SafeCleanupPool();

	UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread: exit()\n"));
}

void FConnectThreadPoolThread::SafeCleanupPool()
{
	FScopeLock lockPool(&ThreadPoolLock);
	for (int32 i = 0; i < ConnectThreadPool.Num(); ++i)
	{
		if (nullptr != ConnectThreadPool[i])
		{
			if (ConnectThreadPool[i]->IsKillDone())
			{
				// this thread was safe stopped
				delete ConnectThreadPool[i];
				ConnectThreadPool[i] = nullptr;
			}
			else {
				UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread: a connect thread hadn't been killed. rank = %d\n"), ConnectThreadPool[i]->GetRankID());

				ConnectThreadPool[i]->KillThread();
				delete ConnectThreadPool[i];
				ConnectThreadPool[i] = nullptr;
			}
		}
	}

	// empty the pool without shrink
	if(ConnectThreadPool.Num() > 0)
		ConnectThreadPool.Empty(ConnectThreadPool.GetSlack());
}


void FConnectThreadPoolThread::SafeCleanupQueue()
{
	if (DestructQueue.IsEmpty())
		return;
	FConnectThread* thread = nullptr;
	while (DestructQueue.Dequeue(thread))
	{
		if (nullptr == thread)
		{
			continue;
		}

		if(!thread->IsExited()) 
		{
			// connection->KillThread only called in this class
			// so if it called, iskilldone must be true(block call)
			// wait for exit
			thread->KillThread();
		}

		// had exited
		delete thread;
		thread = nullptr;
	}
}

bool FConnectThreadPoolThread::KillThread()
{
	if (!bKillDone)
	{
		TimeToDie = true;
		UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread: begin kill thread \n"));
		if (nullptr != Thread)
		{
			// Trigger the thread so that it will come out of the wait state if
			// it isn't actively doing work
			//if(event) event->Trigger();
			UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread: stop \n"));
			Stop();
			UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread: wait exit \n"));
			// If waiting was specified, wait the amount of time. If that fails,
			// brute force kill that thread. Very bad as that might leak.
			Thread->WaitForCompletion();
			UE_LOG(LogTemp, Display, TEXT("FConnectThreadPoolThread: cleanup pools \n"));

			SafeCleanupPool();
			SafeCleanupQueue();

			// Clean up the event
			// if(event) FPlatformProcess::ReturnSynchEventToPool(event);
			// event = nullptr;

			// here will call Stop()
			delete Thread;
			Thread = nullptr;
		}

		bKillDone = true;
	}

	return bKillDone;
}

FConnectThreadPoolThread * FConnectThreadPoolThread::Create(int32 InMaxBacklog, float InPoolTimespan)
{
	FConnectThreadPoolThread* runnable = new FConnectThreadPoolThread(InMaxBacklog);

	runnable->SleepTimeSpan = InPoolTimespan;
	
	// create thread with runnable
	FRunnableThread* thread = FRunnableThread::Create(runnable, TEXT("FConnectThreadPoolThread"), 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify 
	if (nullptr == thread)
	{
		// create failed
		delete runnable;
		return nullptr;
	}

	// setting thread
	runnable->Thread = thread;
	return runnable;
}

void FConnectThreadPoolThread::SafeHoldThread(FConnectThread * InThread)
{
	// pool thread must in run
	check(2 == LifecycleStep.GetValue());

	if (nullptr != InThread)
	{
		FScopeLock lockPool(&ThreadPoolLock);
		ConnectThreadPool.Add(InThread);
	}
}

bool FConnectThreadPoolThread::IsKillDone()
{
	bool ret = bKillDone;
	return ret;
}

int32 FConnectThreadPoolThread::GetLifecycleStep()
{
	return LifecycleStep.GetValue();
}

int32 FConnectThreadPoolThread::GetPoolLength()
{
	FScopeLock lockPool(&ThreadPoolLock);
	return ConnectThreadPool.Num();
}

void FConnectThreadPoolThread::SetCleanupTimespan(float InTimespan)
{
	SleepTimeSpan = InTimespan;
}

float FConnectThreadPoolThread::GetCleanupTimespan()
{
	return SleepTimeSpan;
}
