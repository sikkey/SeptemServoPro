// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "TestServerActor.h"

// Sets default values
ATestServerActor::ATestServerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Port = 3717;
	bListenInit = true;

	ServerThread = nullptr;

	PoolTimespan = 0.05f;

	bCleanup = true;
}

// Called when the game starts or when spawned
void ATestServerActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (bListenInit)
		RunServer();
}

void ATestServerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//release thread
	ShutdownServer();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ATestServerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATestServerActor::RunServer(bool bRestart)
{

	if (bRestart && nullptr != ServerThread) {
		ShutdownServer();
	}

	if (nullptr == ServerThread)
	{
		ServerThread = FListenThread::Create(Port, PoolTimespan);
	}
}

void ATestServerActor::ShutdownServer()
{
	// metion: if ServerThread create failed, the ptr == nullptr
	if (nullptr != ServerThread)
	{
		ServerThread->KillThread();
		delete ServerThread;
		ServerThread = nullptr;
	}
}

int32 ATestServerActor::GetServerLifecycle()
{
	if (ServerThread)
	{
		return ServerThread->GetLifecycleStep();
	}

	return 0;
}

int32 ATestServerActor::GetRankID()
{
	if (ServerThread)
	{
		return ServerThread->GetRankID();
	}

	return 0;
}

int32 ATestServerActor::GetConnectPoolLifecycle()
{
	if (ServerThread)
	{
		return ServerThread->GetPoolLifecycleStep();
	}

	return 0;
}

int32 ATestServerActor::GetConnectPoolLength()
{
	if (ServerThread)
	{
		FConnectThreadPoolThread* poolThread = ServerThread->GetPoolThread();

		if (poolThread)
		{
			return poolThread->GetPoolLength();
		}
	}
	return 0;
}

float ATestServerActor::GetPoolTimespan()
{
	return PoolTimespan;
}

void ATestServerActor::SetPoolTimespan(float InTimespan)
{
	PoolTimespan = InTimespan;
	if (ServerThread)
	{
		FConnectThreadPoolThread* poolThread = ServerThread->GetPoolThread();
		if (poolThread)
		{
			poolThread->SetCleanupTimespan(InTimespan);
			PoolTimespan = poolThread->GetCleanupTimespan();
		}
	}
}

void ATestServerActor::SetNeedCleanup(bool InNeedCleanup)
{
	bCleanup = InNeedCleanup;

	if (ServerThread)
	{
		FConnectThreadPoolThread* poolThread = ServerThread->GetPoolThread();
		if (poolThread)
		{
			UE_LOG(LogTemp, Display, TEXT("ATestServerActor: pool cleanup state change"));
			poolThread->bCleanup = InNeedCleanup;
			bCleanup = poolThread->bCleanup;
		}
	}
}


