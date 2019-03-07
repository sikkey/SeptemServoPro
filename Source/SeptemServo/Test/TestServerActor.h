// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Threads/ListenThread.h"
#include "TestServerActor.generated.h"

UCLASS()
class SEPTEMSERVO_API ATestServerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestServerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Overridable function called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	FListenThread* ServerThread;

public:
	UFUNCTION(BlueprintCallable, Category = "Server")
	void RunServer(bool bRestart = false);

	UFUNCTION(BlueprintCallable, Category = "Server")
		void ShutdownServer();

	UFUNCTION(BlueprintCallable, Category = "Server")
		int32 GetServerLifecycle();

	UFUNCTION(BlueprintCallable, Category = "Server")
		int32 GetConnectPoolLifecycle();

public:
	//		server settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Actor)
		int32 Port;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Actor)
		bool bListenInit;
};
