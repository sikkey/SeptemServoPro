// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ListenThread.h"

FListenThread::FListenThread()
	:FRunnable()
	,TimeToDie(false)
	,Port(3717)
	,Thread(nullptr)
{
}

FListenThread::~FListenThread()
{
	if (nullptr != Thread)
	{
		Thread->Kill(true);
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
}

bool FListenThread::Init()
{
	return false;
}

uint32 FListenThread::Run()
{
	return 0;
}

void FListenThread::Stop()
{
	// call father function
	FRunnable::Stop();
}

void FListenThread::Exit()
{
}
