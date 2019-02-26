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
		ServerThread = FListenThread::Create();
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


