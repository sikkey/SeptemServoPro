// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Private/HAL/PThreadRunnableThread.h"
#include "Networking.h"
#include "ConnectThread.h"

/**
 * 
 */
class SEPTEMSERVO_API FConnectThreadPoolThread : public FRunnable
{
public:
	FConnectThreadPoolThread();
	FConnectThreadPoolThread(int32 InMaxBacklog);
	~FConnectThreadPoolThread();

	// Begin FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	// End FRunnable interface

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

	FThreadSafeCounter LifecycleStep;
	//TAtomic<bool> bPause;  //or FThreadSafeBool bPause;
	//FEvent * Semaphore;

	// main thread
	FRunnableThread* Thread;

	//---------------------------------------------
	// client connections
	//---------------------------------------------
	int32 RankId;	// consider volatile 
	FCriticalSection ThreadPoolLock;
	TArray<FConnectThread*> ConnectThreadPool;

	void SafeCleanupPool();

public:
	//-------------------------------------------------------------------
	// Critical Lock Call By Another Thread
	//-------------------------------------------------------------------
	//~~~ Starting and Stopping Thread ~~~

	/** Makes sure this thread has stopped properly */
	// must use KillThread to void deadlock
	// if you use thread->kill() directly , easy to get deadlock or crash
	bool KillThread();// use KillThread instead of thread->kill
	static FConnectThreadPoolThread* Create(int32 InMaxBacklog = 100);
	void SafeHoldThread(FConnectThread* InThread);

	// state
	bool IsKillDone();

};
