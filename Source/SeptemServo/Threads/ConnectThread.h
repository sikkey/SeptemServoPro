// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Private/HAL/PThreadRunnableThread.h"
#include "Networking.h"

/**
 * 
 */
class SEPTEMSERVO_API FConnectThread : public FRunnable
{
public:
	FConnectThread();
	FConnectThread(FSocket* InSocket, FIPv4Address& InIP, int32 InPort, int32 InRank = 0);
	virtual ~FConnectThread();
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
	static FConnectThread* Create(FSocket* InSocket, FIPv4Address& InIP, int32 InPort, int32 InRank = 0);

private:
	//---------------------------------------------
	// thread control
	//---------------------------------------------

	/** If true, the thread should exit. */
	TAtomic<bool> TimeToDie;

	// if ture means we had called stop();
	FThreadSafeBool bStopped;
	//TAtomic<bool> bPause;  //or FThreadSafeBool bPause;
	//FEvent * Semaphore;

	// main thread
	FRunnableThread* Thread;

	//---------------------------------------------
	// connection settings
	//---------------------------------------------
	
	FSocket* ConnectSocket;				// socket
	FIPv4Address ClientIPAdress;		// client ip
	int32 Port;										// client Port
	int32 RankId;									// rank id
};
