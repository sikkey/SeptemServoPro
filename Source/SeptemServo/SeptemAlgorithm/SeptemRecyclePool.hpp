// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Templates/SharedPointerInternals.h"
//#include "Containers/BinaryHeap.h"

namespace Septem
{
	/*
	*	SharedPtr Recycle Pool
	*	default mod = no thread-safe
	*	please use ESPMode::ThreadSafe for thread-safe
	*/
	template<typename T, ESPMode InMode = ESPMode::Fast>
	class SEPTEMSERVO_API TSharedRecyclePool
	{
	public:
		TSharedRecyclePool(int32 InNum = 1024)
		{
			if (InMode == ESPMode::ThreadSafe)
			{
				poolCriticalSection.Lock();
			}

			ptrPool.Reset(InNum);
			
			for (int32 i = 0; i < InNum; ++i)
			{
				TSharedPtr<T, InMode> ptr(new T());
				ptrPool.Add(ptr);
			}

			if (InMode == ESPMode::ThreadSafe)
			{
				poolCriticalSection.Unlock();
			}
		}

		~TSharedRecyclePool()
		{}

		// Thread safe when InMode = ESPMode::ThreadSafe
		TSharedPtr<T, InMode> Alloc()
		{
			if (InMode == ESPMode::ThreadSafe)
			{
				poolCriticalSection.Lock();
				if (ptrPool.Num() > 0)
				{				
					//TSharedPtr<T, InMode> popPtr(ptrPool.Pop(false));
					TSharedPtr<T, InMode>&& popPtr = ptrPool.Pop(false);
					poolCriticalSection.Unlock();

					return popPtr;
				}
				poolCriticalSection.Unlock();

				return TSharedPtr<T, InMode>(new T());
			}
			return PopOrCreate();
		}

		/* 
		*	Thread safe when InMode = ESPMode::ThreadSafe
		*	ensure the InSharedPtr is valid
		*/
		void Dealloc(const TSharedPtr<T, InMode>& InSharedPtr)
		{
			if (InSharedPtr.IsValid())
			{
				if (InMode == ESPMode::ThreadSafe)
				{
					FScopeLock lockPool(&poolCriticalSection);
					if (ptrPool.GetSlack() > 0)
					{
						ptrPool.Push(InSharedPtr);
					}
				}
				else {
					if (ptrPool.GetSlack() > 0)
					{
						ptrPool.Push(InSharedPtr);
					}
				}
			}
		}

		/*
		*	Thread safe when InMode = ESPMode::ThreadSafe
		*	ensure the InSharedPtr is valid
		*/
		void DeallocForceRecycle(const TSharedPtr<T, InMode>& InSharedPtr)
		{
			if (InSharedPtr.IsValid()) {
				if (InMode == ESPMode::ThreadSafe)
				{
					FScopeLock lockPool(&poolCriticalSection);
					ptrPool.Push(InSharedPtr);
				}
				else {
					ptrPool.Push(InSharedPtr);
				}
			}
		}

		// not thread safe
		int32 Num()
		{
			return ptrPool.Num();
		}

	private:

		// not thread safe
		FORCEINLINE TSharedPtr<T, InMode> PopOrCreate()
		{
			if (ptrPool.Num() > 0)
			{
				return ptrPool.Pop(false);
			}

			return TSharedPtr<T, InMode>(new T());
		}

		// stack of shared ptr object 
		// ensure every sharedptr in pool is valid
		TArray< TSharedPtr<T, InMode> >  ptrPool;
		FCriticalSection poolCriticalSection;
	};
}