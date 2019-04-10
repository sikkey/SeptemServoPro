// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "TemplateThread.h"

FTemplateThread::FTemplateThread()
	:FRunnable()
	, TimeToDie(false)
	, bKillDone(false)
{
}

FTemplateThread::~FTemplateThread()
{
	if (LifecycleStep.GetValue() == 2)
	{
		UE_LOG(LogTemp, Display, TEXT("FTemplateThread destruct: cannot exit safe"));
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
}

bool FTemplateThread::Init()
{
	LifecycleStep.Set(1);
	// if init success, return true here
	return false;
}

uint32 FTemplateThread::Run()
{
	LifecycleStep.Set(2);
	//FPlatformMisc::MemoryBarrier();

	/*
	// [Warnning] Mustn't use bStopped here!
	while(!TimeToDie)
	{
		// tick run
	}
	*/

	// ExitCode:0 means no error
	return 0;
}

void FTemplateThread::Stop()
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

void FTemplateThread::Exit()
{
	LifecycleStep.Set(3);
	// cleanup Run() ptr;
}

/**
 * Tells the thread to exit. If the caller needs to know when the thread
 * has exited, it should use the bShouldWait value and tell it how long
 * to wait before deciding that it is deadlocked and needs to be destroyed.
 * NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
 *
 * @return True if the thread exited graceful, false otherwise
 */
bool FTemplateThread::KillThread()
{
	if (bKillDone)
	{
		TimeToDie = true;

		if (nullptr != Thread)
		{
			// Trigger the thread so that it will come out of the wait state if
			// it isn't actively doing work
			//if(event) event->Trigger();

			// If waiting was specified, wait the amount of time. If that fails,
			// brute force kill that thread. Very bad as that might leak.
			Thread->WaitForCompletion();

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

FTemplateThread * FTemplateThread::Create()
{
	FTemplateThread* runnable = new FTemplateThread();
	// create thread with runnable
	FRunnableThread* thread = FRunnableThread::Create(runnable, TEXT("FTemplateThread"), 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify 
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

bool FTemplateThread::IsKillDone()
{
	bool ret = bKillDone;
	return ret;
}
