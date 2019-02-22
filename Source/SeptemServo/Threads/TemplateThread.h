// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Private/HAL/PThreadRunnableThread.h"
#include "Networking.h"

/***************class/type/value for thread***************************

	std::atomic<type>		=>	TAtomic<type>
	std::promise<int32>	=>	TPromise<int32>
	std::future<T>				=>	TFuture<T>
	std::thread					=>	FRunnableThread

	atomic:
	TAtomic<T>							T
	FThreadSafeBool					bool
	FThreadSafeCounter				int32
	FThreadSafeCounter64			int64

	mutex:
	std::mutex				=>	FCriticalSection
	std::shared_mutex	=>	FRWLock

	kill:
	pFRunnableThread->Kill(true) will call pFRunnable->Stop()
	see WindowsRunnableThread.h

	control
	use FPlatformProcess::Sleep(x) to sleep x seconds

	etc:
	FEvent *  semaphore;  // for pause and trigger a thread
	use FScopeLock ScopeLock(&CriticalSection); to lock in a scope
	use FPlatformMisc::MemoryBarrier(); for sync and var volatile
	This typically means that operations issued prior to the barrier are guaranteed to be performed before operations issued after the barrier.

	reference:
	FQueuedThread
	FQueuedThreadPool
	https://software.intel.com/en-us/forums/intel-c-compiler/topic/393396
	https://en.wikipedia.org/wiki/Memory_barrier
	https://usagi.hatenablog.jp/entry/2017/06/10/122720
	https://usagi.hatenablog.jp/entry/2017/12/01/ac_ue4_2_p5

***************************************************************************/


/**
 * 
 */
class SEPTEMSERVO_API FTemplateThread : public FRunnable
{
public:
	FTemplateThread();
	virtual ~FTemplateThread();

	// Begin FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	// End FRunnable interface

	//~~~ Starting and Stopping Thread ~~~

	/** Makes sure this thread has stopped properly */
	// must use KillThread to void deadlock
	// if you use thread->kill() directly , easy to get deadlock or crash
	bool KillThread();// use KillThread instead of thread->kill
	static FTemplateThread* Create();

	// state
	bool IsKillDone();
private:
	//---------------------------------------------
	// thread control
	//---------------------------------------------

	/** If true, the thread should exit. */
	TAtomic<bool> TimeToDie;

	// if ture means we had called stop();
	FThreadSafeBool bStopped;

	// thread had killed, so there is no run
	FThreadSafeBool bKillDone;

	// show the step of lifecycle
	// 0  construct
	// 1: init
	// 2: run
	// 3: exit
	FThreadSafeCounter LifecycleStep;
	//TAtomic<bool> bPause;  //or FThreadSafeBool bPause;
	//FEvent * Semaphore;

	// main thread
	FRunnableThread* Thread;
};
