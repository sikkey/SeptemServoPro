// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Private/HAL/PThreadRunnableThread.h"
#include "Networking.h"

/**
 * 
 */
class SEPTEMSERVO_API FListenThread : public FRunnable
{
public:
	FListenThread();
	virtual ~FListenThread();

	virtual bool Init() override;

	// 
	virtual uint32 Run() override;

	virtual void Stop() override;

	virtual void Exit() override;

private:
	//---------------------------------------------
	// thread control
	//---------------------------------------------

	/** If true, the thread should exit. */
	TAtomic<bool> TimeToDie;

	//---------------------------------------------
	// server config
	//---------------------------------------------
	FIPv4Endpoint ServerIPv4EndPoint;
	FIPv4Address IPAdress;
	int32 Port;

	// server listen thread
	FRunnableThread* Thread;

	//---------------------------------------------
	// client connections
	//---------------------------------------------
};

