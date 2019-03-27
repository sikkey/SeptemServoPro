// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Containers/BinaryHeap.h"

#define MAX_NETPACKET_IN_POOL 1024

/**
 * net packet pool base class
 * for set any pool algorithm 
 */
template<typename T, ESPMode TMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketPool
{
public:
	FORCEINLINE TNetPacketPool() = default;
	virtual ~TNetPacketPool()
	{
	}

	/**
	 * Push an item to the pool.
	 * @Thread-safe
	 * @param InSharedPtr The item to add.
	 * @return true if the item was added, false otherwise.
	 * @note To be called only from producer thread(s).
	 * @see Pop
	 */
	virtual bool Push(const TSharedPtr<T, TMode>& InSharedPtr) = 0;
	/**
	 * Removes and returns the item from the tail of the pool.
	 * @Thread-safe
	 * @param OutSharedPtr Will hold the returned value.
	 * @return true if a value was returned, false if the pool was empty.
	 * @note To be called only from consumer thread.
	 * @see Push
	 */
	virtual bool Pop(TSharedPtr<T, TMode>& OutSharedPtr) = 0;
	// may not Thread-safe
	virtual bool IsEmpty() = 0;
};

/**
 * net packet pool with Stack strategy
 * Attention about maxnum > heap.num
 * add a private lock for Multiple-producers
 */
template<typename T, ESPMode TMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketStack
{
public:
	FORCEINLINE TNetPacketStack()
		: TNetPacketPool()
	{
		FScopeLock lockPool(&StackPool);
		StackPool.Reset(MAX_NETPACKET_IN_POOL);
	}

	virtual ~TNetPacketStack()
	{
		FScopeLock lockPool(&StackPool);
		StackPool.Empty(StackPool.Max());
	}

	// Thread-safe
	virtual bool Push(const TSharedPtr<T, TMode>& InSharedPtr) override
	{
		FScopeLock lockPool(&StackPool);
		//StackPool.Push(InSharedPtr);
		StackPool.Emplace(InSharedPtr);
		return true;
	}
	// Thread-safe
	virtual bool Pop(TSharedPtr<T, TMode>& OutSharedPtr) override
	{
		FScopeLock lockPool(&StackPool);
		if (IsEmpty())
			return false;
		OutSharedPtr = StackPool.Pop(false);
		return true;
	}
	// may not Thread-safe
	virtual bool IsEmpty() override
	{
		return StackPool.Num() == 0;
	}

private:
	TArray<TSharedPtr<T, TMode> > StackPool;
	FCriticalSection StackLock;
};

/**
 * net packet pool with Queue strategy
 * Multiple-producers single-consumer (MPSC)  for multi-thread
 */
template<typename T, ESPMode TMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketQueue
	: public TNetPacketPool<T, TMode>
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

	virtual bool Push(const TSharedPtr<T, TMode>& InSharedPtr) override
	{
		return packetPool.Enqueue(InSharedPtr);
	}

	virtual bool Pop(TSharedPtr<T, TMode>& OutSharedPtr) override
	{
		return packetPool.Dequeue(OutSharedPtr);
	}

	virtual bool IsEmpty() override
	{
		return packetPool.IsEmpty();
	}

private:
	TQueue<TSharedPtr<T, TMode>, EQueueMode::Mpsc > packetPool;
};


/**
 * net packet pool with Heap strategy
 * Attention about maxnum > heap.num
 * add a private lock for Multiple-producers 
 */
template<typename T, ESPMode TMode = ESPMode::Fast>
class SEPTEMSERVO_API TNetPacketHeap
	: public TNetPacketPool<T, TMode>
{
public:
	FORCEINLINE TNetPacketHeap()
		: TNetPacketPool()
	{
		FScopeLock lockPool(&HeapLock);
		heapPool.Reset(MAX_NETPACKET_IN_POOL);
	}

	virtual ~TNetPacketHeap()
	{
		FScopeLock lockPool(&HeapLock);
		heapPool.Empty(heapPool.Max());
	}

	bool Push(const TSharedPtr<T, TMode>& InSharedPtr) override
	{
		FScopeLock lockPool(&HeapLock);
		return heapPool.HeapPush(InSharedPtr) >=0;
	}

	bool Pop(TSharedPtr<T, TMode>& OutSharedPtr) override
	{
		FScopeLock lockPool(&HeapLock);
		if (IsEmpty())
			return false;
		heapPool.HeapPop(OutSharedPtr, false);
		return true;
	}

	virtual bool IsEmpty() override
	{
		return heapPool.Num() == 0;
	}
	
private:
	TArray<TSharedPtr<T, TMode> > heapPool;
	FCriticalSection HeapLock;
};