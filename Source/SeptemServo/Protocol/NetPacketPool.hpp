// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Containers/BinaryHeap.h"

/**
 * net packet pool base class
 * for set any pool algorithm 
 */
template<typename T>
class SEPTEMSERVO_API TNetPacketPool
{
public:
	FORCEINLINE TNetPacketPool() = default;
	virtual ~TNetPacketPool()
	{
		packetPool.Empty();
	}

	virtual bool Push(const TSharedPtr<T>& InSharedPtr) = 0;
	virtual bool Pop(TSharedPtr<T>& OutSharedPtr) = 0;
	virtual bool IsEmpty() = 0;
};

/**
 * net packet pool with Queue strategy
 * Multiple-producers single-consumer (MPSC)  for multi-thread
 */
template<typename T>
class SEPTEMSERVO_API TNetPacketQueue
	: public TNetPacketPool<T>
{
	FORCEINLINE TNetPacketQueue()
		: TNetPacketPool()
	{
	}

	virtual ~TNetPacketQueue()
	{
		packetPool.Empty();
	}

	bool Push(const TSharedPtr<T>& InSharedPtr) override
	{
		return packetPool.Enqueue(InSharedPtr);
	}

	bool Pop(TSharedPtr<T>& OutSharedPtr) override
	{
		return packetPool.Dequeue(OutSharedPtr);
	}

	virtual bool IsEmpty() override
	{
		return packetPool.IsEmpty();
	}

private:
	TQueue< TSharedPtr<T>, EQueueMode::Mpsc > packetPool;
};


/**
 * net packet pool with Heap strategy
 * Attention about maxnum > heap.num
 * add a private lock for Multiple-producers 
 */
template<typename T>
class SEPTEMSERVO_API TNetPacketHeap
	: public TNetPacketPool<T>
{
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

	bool Push(const TSharedPtr<T>& InSharedPtr) override
	{
		FScopeLock lockPool(&HeapLock);
		return heapPool.HeapPush(InSharedPtr) >=0;
	}

	bool Pop(TSharedPtr<T>& OutSharedPtr) override
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
	TArray<TSharedPtr<T> > heapPool;
	FCriticalSection HeapLock;
};