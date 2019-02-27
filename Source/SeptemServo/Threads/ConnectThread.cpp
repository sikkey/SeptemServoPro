// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ConnectThread.h"

// default buffer max  = 1mb
int32 FConnectThread::MaxReceivedCount = 1024 * 1024;

FConnectThread::FConnectThread()
	:FRunnable()
	, ConnectSocket(nullptr)
	, ClientIPAdress(0ui32)
	, Port(3717)
	, RankId(0)
{
	ReceivedData.Reset(MaxReceivedCount);
}

FConnectThread::FConnectThread(FSocket * InSocket, FIPv4Address & InIP, int32 InPort, int32 InRank)
	:FRunnable()
	, ConnectSocket(InSocket)
	, ClientIPAdress(InIP)
	, Port(InPort)
	, RankId(InRank)
{
	ReceivedData.Reset(MaxReceivedCount);
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

	// cleanup socket
	SafeDestorySocket();
}

bool FConnectThread::Init()
{
	LifecycleStep.Set(1);
	// if init success, return true here
	return true;
}

uint32 FConnectThread::Run()
{
	LifecycleStep.Set(2);
	//FPlatformMisc::MemoryBarrier();
	uint32 pendingDataSize = 0;
	int32 BytesRead = 0;
	bool bRcev = false;

	if (nullptr == ConnectSocket)
	{
		return 1ui32;  //exit code == 1 : thread run failed
	}

	while (!TimeToDie)
	{
		if (ConnectSocket->HasPendingData(pendingDataSize) && pendingDataSize > 0)
		{
			BytesRead = 0;
			bRcev = false;

			bRcev = ConnectSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, ESocketReceiveFlags::None);

			if (bRcev && BytesRead > 0)
			{
				if (BytesRead == ReceivedData.Num())
				{
					UE_LOG(LogTemp, Display, TEXT("[Warnning]FConnectThread: receive stack overflow!\n"));
					//TODO: stack overflow
					continue;
				}

				// TODO: recv data
				UE_LOG(LogTemp, Display, TEXT("FConnectThread: receive chars = %s length = %d\n"), ReceivedData.GetData(), ReceivedData.Num());
			}

			// TODO: do with no pending data
			UE_LOG(LogTemp, Display, TEXT("FConnectThread: no pending data!!!\n"));
			
			// TODO: check disconnect
		}
	}

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

		if (nullptr != ConnectSocket)
		{
			ConnectSocket->Shutdown(ESocketShutdownMode::ReadWrite);
		}

		bStopped = true;
	}
}

void FConnectThread::Exit()
{
	LifecycleStep.Set(3);
	// cleanup socket
	SafeDestorySocket();
	UE_LOG(LogTemp, Display, TEXT("FConnectThread: exit()\n"));

	LifecycleStep.Set(4);
}

bool FConnectThread::KillThread()
{
	// bKillDone maybe 0
	if (1 == bKillDone.Increment()) {
		// bKillDone at least 1, thread safe
		TimeToDie = true;

		if (nullptr != Thread)
		{
			// close socket
			/*
			if (nullptr != ConnectSocket && !bStopped)
			{
				ConnectSocket->Shutdown(ESocketShutdownMode::ReadWrite);
			}
			*/
			Stop();

			// Trigger the thread so that it will come out of the wait state if
			// it isn't actively doing work
			//if(event) event->Trigger();

			// Block until this thread exits()
			Thread->WaitForCompletion();

			// Clean up the event
			// if(event) FPlatformProcess::ReturnSynchEventToPool(event);
			// event = nullptr;

			// here will call Stop() again for safe close handle
			delete Thread;
			Thread = nullptr;
		}

		// make sure bKillDone at least 2, thread safe
		bKillDone.Increment();
	}

	if (bKillDone.GetValue() > 100)
	{
		// defend killdone overflow
		bKillDone.Set(2);
	}

	return bKillDone.GetValue() > 1;
}

FConnectThread * FConnectThread::Create(FSocket * InSocket, FIPv4Address & InIP, int32 InPort, int32 InRank)
{
	FConnectThread* runnable = new FConnectThread(InSocket, InIP, InPort, InRank);
	// create thread with runnable
	FString threadName = FString::Printf(TEXT("FConnectThread%d"), InRank);

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

bool FConnectThread::IsSocketConnection()
{
	if (ConnectSocket)
	{	// TODO: FIX BUG cause fatal error here, address = 0xffffffff
		if (ConnectSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
		{
			return true;
		}
	}

	UE_LOG(LogTemp, Display, TEXT("FConnectThread: detect disconnect\n"));

	return false;
}

bool FConnectThread::IsKillCalled()
{
	return bKillDone.GetValue() > 0;
}

bool FConnectThread::IsKillDone()
{
	return bKillDone.GetValue() > 1;
}

bool FConnectThread::IsExited()
{
	return LifecycleStep.GetValue() >= 4;
}

int32 FConnectThread::GetRankID() const
{
	return RankId;
}

void FConnectThread::SafeDestorySocket()
{
	if (nullptr != ConnectSocket)
	{
		ConnectSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectSocket);
		ConnectSocket = nullptr;
	}
}



