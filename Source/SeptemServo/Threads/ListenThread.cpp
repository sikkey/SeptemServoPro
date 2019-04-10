// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ListenThread.h"

FListenThread::FListenThread()
	:FRunnable()
	, TimeToDie(false)
	, bStopped(false)
	, IPAdress(0ui32) // default ip = 0.0.0.0
	, Port(3717)
	, MaxBacklog(100)
	, PoolTimespan(0)
	, ListenerSocket(nullptr)
	, Thread(nullptr)
	, RankId(0)
	, ConnectionPoolThread(nullptr)
{
}

FListenThread::~FListenThread()
{
	// cleanup thread
	if (nullptr != Thread)
	{
		delete Thread;
		Thread = nullptr;
	}
	// cleanup events
	// null events here

	// cleanup init ptr
	SafeDestorySocket();
}

bool FListenThread::Init()
{
	LifecycleStep.Set(1);
	// do network 
	
	// 1. create socket
	ListenerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("listen socket"), false);
	//check(ListenerSocket);

	if (nullptr == ListenerSocket)
	{
		UE_LOG(LogTemp, Display, TEXT("ListenerSocket: failed to create socket\n"));
		return false;
	}

	// 2. bind socket
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(IPAdress.Value);
	addr->SetPort(Port);
	bool bBind = ListenerSocket->Bind(*addr);

	//check(bBind);

	if (!bBind)
	{
		UE_LOG(LogTemp, Display, TEXT("ListenerSocket: failed to bind port = %d\n"), Port);
		return false;
	}

	// 3. listen socket

	bool bListen = ListenerSocket->Listen(MaxBacklog);

	UE_LOG(LogTemp, Display, TEXT("ListenerSocket: server listening\n"));

	// <----- insert:  create connection pool
	SafeConstructConnectionPool();

	// 4. accept client
	// 5. recv data
	// accept&recv task in Run()
	return true;
}

uint32 FListenThread::Run()
{
	LifecycleStep.Set(2);
	bool bHasPendingConnection = false;
	if (nullptr == ListenerSocket) return 1ui32;  //exit code == 1 : thread run failed
	while (!TimeToDie)
	{
		if (ListenerSocket->HasPendingConnection(bHasPendingConnection))
		{
			if (!bHasPendingConnection)
			{
				//no pending connection
				//FPlatformProcess::Sleep(0.02f);
				continue;
			}

			//FPlatformMisc::MemoryBarrier();

			TSharedRef<FInternetAddr> clientAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			// accept with description = client {Rank Id}
			FSocket* ConnectSocket = ListenerSocket->Accept(*clientAddr, FString::Printf(TEXT("ListenServer%d"), RankId));

			if(nullptr == ConnectSocket)
			{
				UE_LOG(LogTemp, Display, TEXT("ListenerSocket: accept null socket, listener cannot read and write \n"));
				// break and close the thread
				return 1ui32;
			}

			UE_LOG(LogTemp, Display, TEXT("ListenerSocket: connect socket ptr = %d  ip =  %s\n"), ConnectSocket, *FIPv4Endpoint(clientAddr).ToString());

			//check(ConnectSocket && "ConnectSocket == nullptr");
			// connectThread will hold the ConnectSocket ptr, Don't care about it in this thread
			ConnectSocket->SetNonBlocking(true);

			FPlatformMisc::MemoryBarrier();
			FIPv4Endpoint endPoint(clientAddr);

			UE_LOG(LogTemp, Display, TEXT("ListenerSocket: accept client ip = %s \n"), *endPoint.ToString());

			FConnectThread* connectThread = FConnectThread::Create(ConnectSocket, endPoint.Address, endPoint.Port, RankId);
			if (connectThread != nullptr)
			{
				ConnectionPoolThread->SafeHoldThread(connectThread);
				++RankId;
			}
		}
		else {
			UE_LOG(LogTemp, Display, TEXT("ListenerSocket: throw error when pending connection\n"));
		}
	}
	//FPlatformMisc::MemoryBarrier();
	return 0;
}

void FListenThread::Stop()
{
	if (!bStopped) {
		TimeToDie = true;
		// call father function
		//FRunnable::Stop(); // father function == {}

		// because pthread->kill will call stop
		// IMPORTANT!!!you cannot call pthread->kill here!!!

		if (nullptr != ListenerSocket)
		{
			ListenerSocket->Shutdown(ESocketShutdownMode::ReadWrite);
		}

		bStopped = true;
	}
}

void FListenThread::Exit()
{
	LifecycleStep.Set(3);
	SafeDestructConnectionPool();
	// cleanup socket
	SafeDestorySocket();
	UE_LOG(LogTemp, Display, TEXT("FListenThread: exit()\n"));
	LifecycleStep.Set(4);
}

/**
 * Tells the thread to exit. If the caller needs to know when the thread
 * has exited, it should use the bShouldWait value and tell it how long
 * to wait before deciding that it is deadlocked and needs to be destroyed.
 * NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
 *
 * @return True if the thread exited graceful, false otherwise
 */
bool FListenThread::KillThread()
{
	bool bDidExit = true;

	TimeToDie = true;

	if (nullptr != Thread)
	{
		// Trigger the thread so that it will come out of the wait state if
		// it isn't actively doing work
		//if(event) event->Trigger();

		Stop();

		// If waiting was specified, wait the amount of time. If that fails,
		// brute force kill that thread. Very bad as that might leak.
		Thread->WaitForCompletion();	//block call

		// Clean up the event
		// if(event) FPlatformProcess::ReturnSynchEventToPool(event);
		// event = nullptr;

		// here will call Stop()
		delete Thread;
		Thread = nullptr;

		// socket had been safe release in exit();
	}

	return bDidExit;
}

FListenThread * FListenThread::Create(int32 InPort, float InPoolTimespan)
{
	// if you need create event
	//Event = FPlatformProcess::GetSynchEventFromPool();

	// create runnable
	FListenThread* runnable = new FListenThread();
	runnable->Port = InPort;
	runnable->PoolTimespan = InPoolTimespan;

	// create thread with runnable
	FRunnableThread* thread = FRunnableThread::Create(runnable, TEXT("FListenThread"), 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify 
	
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

int32 FListenThread::GetLifecycleStep()
{
	return LifecycleStep.GetValue();
}

int32 FListenThread::GetPoolLifecycleStep()
{
	if (ConnectionPoolThread)
	{
		return ConnectionPoolThread->GetLifecycleStep();
	}
	return 0;
}

FConnectThreadPoolThread * FListenThread::GetPoolThread()
{
	return ConnectionPoolThread;
}

int32 FListenThread::GetRankID()
{
	return RankId;
}

void FListenThread::SafeDestorySocket()
{
	if (nullptr != ListenerSocket)
	{
		ListenerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
	}
}

void FListenThread::SafeConstructConnectionPool()
{
	if (nullptr == ConnectionPoolThread)
	{ 
		ConnectionPoolThread = FConnectThreadPoolThread::Create(MaxBacklog, PoolTimespan);
		UE_LOG(LogTemp, Display, TEXT("ListenerSocket: init connection pool \n"));
	}
}

void FListenThread::SafeDestructConnectionPool()
{
	if (nullptr != ConnectionPoolThread)
	{
		ConnectionPoolThread->KillThread();
		UE_LOG(LogTemp, Display, TEXT("ListenerSocket: Destruct connection pool \n"));
	}
}

