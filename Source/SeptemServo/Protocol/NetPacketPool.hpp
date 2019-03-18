// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Containers/BinaryHeap.h"

/**
 * net packet pool base class
 * for set any pool algorithm 
 */
template<typename T, ESPMode InMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketPool
{
public:
	FORCEINLINE TNetPacketPool() = default;
	virtual ~TNetPacketPool()
	{
	}

	// Thread-safe
	virtual bool Push(const TSharedPtr<T, InMode>& InSharedPtr) = 0;
	// Thread-safe
	virtual bool Pop(TSharedPtr<T, InMode>& OutSharedPtr) = 0;
	// may not Thread-safe
	virtual bool IsEmpty() = 0;
};

/**
 * net packet pool with Queue strategy
 * Multiple-producers single-consumer (MPSC)  for multi-thread
 */
template<typename T, ESPMode InMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketQueue
	: public TNetPacketPool<T, InMode>
{
public:
	FORCEINLINE TNetPacketQueue()
		: TNetPacketPool()
	{
	}

	virtual ~TNetPacketQueue()
	{
		packetPool.Empty();
	}

	virtual bool Push(const TSharedPtr<T, InMode>& InSharedPtr) override
	{
		return packetPool.Enqueue(InSharedPtr);
	}

	virtual bool Pop(TSharedPtr<T, InMode>& OutSharedPtr) override
	{
		return packetPool.Dequeue(OutSharedPtr);
	}

	virtual bool IsEmpty() override
	{
		return packetPool.IsEmpty();
	}

private:
	TQueue<TSharedPtr<T, InMode>, EQueueMode::Mpsc > packetPool;
};


/**
 * net packet pool with Heap strategy
 * Attention about maxnum > heap.num
 * add a private lock for Multiple-producers 
 */
template<typename T, ESPMode InMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketHeap
	: public TNetPacketPool<T, InMode>
{
public:
	FORCEINLINE TNetPacketHeap()
		: TNetPacketPool()
	{
		FScopeLock lockPool(&HeapLock);
		heapPool.Reset(1024);
	}

	virtual ~TNetPacketHeap()
	{
		FScopeLock lockPool(&HeapLock);
		heapPool.Empty();
	}

	bool Push(const TSharedPtr<T, InMode>& InSharedPtr) override
	{
		FScopeLock lockPool(&HeapLock);
		return heapPool.HeapPush(InSharedPtr) >=0;
	}

	bool Pop(TSharedPtr<T, InMode>& OutSharedPtr) override
	{
		FScopeLock lockPool(&HeapLock);
		if (IsEmpty())
			return false;
		heapPool.HeapPop(OutSharedPtr, false);
		return true;
	}

	virtual bool IsEmpty() override
	{
		return heapPool.Num() > 0;
	}
	
private:
	TArray<TSharedPtr<T, InMode> > heapPool;
	FCriticalSection HeapLock;
};