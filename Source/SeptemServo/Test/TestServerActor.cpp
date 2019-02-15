// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "TestServerActor.h"

// Sets default values
ATestServerActor::ATestServerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATestServerActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATestServerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

