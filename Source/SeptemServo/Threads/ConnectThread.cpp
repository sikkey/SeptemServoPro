// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ConnectThread.h"

FConnectThread::FConnectThread()
	:FRunnable()
	, ConnectSocket(nullptr)
	, ClientIPAdress(0ui32)
	, Port(3717)
	, RankId(0)
{
}

FConnectThread::FConnectThread(FSocket * InSocket, FIPv4Address & InIP, int32 InPort, int32 InRank)
	:FRunnable()
	, ConnectSocket(InSocket)
	, ClientIPAdress(InIP)
	, Port(InPort)
	, RankId(InRank)
{
}

FConnectThread::~FConnectThread()
{
	// cleanup thread
	if (nullptr != Thread)
	{
		delete Thread;
		Thread = nullptr;
	}
	// cleanup events
	// null events here
}

bool FConnectThread::Init()
{
	// if init success, return true here
	return false;
}

uint32 FConnectThread::Run()
{
	//FPlatformMisc::MemoryBarrier();

	/*
	while(!TimeToDie)
	{
		// TODO: pending data
		// TODO: recv data
		// TODO: check disconnect
	}
	*/

	// ExitCode:0 means no error
	return 0;
}

void FConnectThread::Stop()
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

void FConnectThread::Exit()
{
	// cleanup socket
	if (nullptr != ConnectSocket)
	{
		delete ConnectSocket;
		ConnectSocket = nullptr;
	}
	UE_LOG(LogTemp, Display, TEXT("FConnectThread: exit()\n"));
}

bool FConnectThread::KillThread()
{
	bool bDidExit = true;

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
		
		// close socket
		//TODO: check socket close
		ConnectSocket->Shutdown(ESocketShutdownMode::ReadWrite);
		ConnectSocket->Close();
	}

	return bDidExit;
}

FConnectThread * FConnectThread::Create(FSocket * InSocket, FIPv4Address & InIP, int32 InPort, int32 InRank)
{
	FConnectThread* runnable = new FConnectThread(InSocket, InIP, InPort, InRank);
	// create thread with runnable
	FString threadName = FString::Printf("FConnectThread%d", InRank); //FString::Printf(TEXT("FConnectThread%d"), InRank);

	FRunnableThread* thread = FRunnableThread::Create(runnable, *threadName, 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify 
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



