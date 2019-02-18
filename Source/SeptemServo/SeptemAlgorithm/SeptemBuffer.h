// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Septem
{
	// template for buffer strstr
	// find N in M, return the first index or  -1 when faied.
	// BufferBuffer<TChar> ( strP1, strlen1, strP2, strlen2);
	// BufferBuffer<uint8> (Buf1, BLen1, Buf2, Blen2);
	// input:
	// MBuffer		:Buffer M
	// MLength		:Buffer M length
	// NBuffer			:Buffer N
	// NLength		:Buffer N length
	// return int32	: the first index or  -1 when faied.
	template<typename T>
	int32 BufferBuffer(T * MBuffer, const int32 MLength, T * NBuffer, const int32 NLength)
	{
		const int32 FAIL_CODE = -1;
		int32 IndexM = 0; // i
		int32 IndexN = 0; // j
		int32 Next[NLength] = { 0 };

		if (MLength <= 0 || NLength <= 0)
			return FAIL_CODE;

		check(MBuffer);
		check(NBuffer);

		// TODO:get next

		// IndexN - NLength <= MLength - IndexM
		// makesure IndexM < MLength && IndexN < NLength
		while( IndexM + NLength <= IndexN + MLength)
		{
Flag_Compare:
			// here must be: M(i - j)~M(i - 1) == N0~N(j - 1)
			// so compare Mi =?= Nj
			if ((*(MBuffer + IndexM)) == (*(NBuffer + IndexN)))
			{
				++IndexN;
				// Mi == N(j-1)
				if (IndexN == NLength)
				{
					// means M(i - n + 1)~Mi == N0~N(n - 1)
					// j == n, found the index = i-j+1
					return IndexM - NLength + 1;
				}
				++IndexM;

				if (IndexM == MLength)
					return FAIL_CODE;
			}
			else {
				// Mi =/= Nj
				if (IndexN > 0)
				{
					IndexN = Next[IndexN]; // j< next(j)
					goto Flag_Compare;
				}
				else {
					// 0==j,  Mi != N0
					++i;
				}
			}
		}

		return FAIL_CODE;
	}
}

