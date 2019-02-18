// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/***************************************/
// net buffer design:
// buffer head
// buffer body
// (optional) buffer foot
/***************************************/

struct FSNetBufferHead
{
	int32 syncword;
	uint8 version;
	uint8 fastcode;  // xor value without this byte
	uint16 uid; // class id
	int32 size; // body size
	int32 reserved; // 64x alignment 

	FSNetBufferHead() :
		syncword(0xE6B7F1A2),
		version(0),
		fastcode(0),
		uid(0),
		size(0),
		reserved(0)
	{
	}
};

// Optional TODO
union UnionBufferHead
{
	uint8 byte[16]; // 4x 32bit
	FSNetBufferHead bufferHead;
};

struct FSNetBufferBody
{
	uint8* bufferPtr;
	int32 length; // lenght == BufferHead.size;

	FSNetBufferBody()
		: bufferPtr(nullptr)
		, length(0)
	{}

	bool IsValid();
};

struct FSNetBufferFoot
{
	FSHA256Signature signature;
	int64 timestamp; // unix timestamp
};

/**
 * the protocol of SeptemServo
 */
class SEPTEMSERVO_API ServoProtocol
{
public:
	ServoProtocol();
	~ServoProtocol();
};
