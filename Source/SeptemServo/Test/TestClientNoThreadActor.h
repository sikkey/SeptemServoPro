// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking.h"
#include "TestClientNoThreadActor.generated.h"

UCLASS()
class SEPTEMSERVO_API ATestClientNoThreadActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestClientNoThreadActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Overridable function called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Actor)
		FString ServerIP;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Actor)
		int32 Port;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Actor)
		bool bStartGameConnect;

	FIPv4Endpoint ServerEndPoint;

	FSocket* ClientSocket;

public:
	UFUNCTION(BlueprintCallable, Category = "Client")
		void ConnectToServer();

	UFUNCTION(BlueprintCallable, Category = "Client")
		void Disconnect();

	UFUNCTION(BlueprintCallable, Category = "Client")
		void ReleaseSocket();

	UFUNCTION(BlueprintCallable, Category = "Client")
		void SendByte(uint8 InByte);
};
