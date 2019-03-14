// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

//#define SERVO_PROTOCOL_SIGNATURE

#ifndef DEFAULT_SYNCWORD_INT32
#define DEFAULT_SYNCWORD_INT32 0xE6B7F1A2
#endif // !DEFAULT_SYNCWORD_INT32


/***************************************/
// net buffer design:
// buffer head
// buffer body
// (optional) buffer foot
/***************************************/
#pragma pack(push, 1)
struct FSNetBufferHead
{
	int32 syncword; // combine with 4 char(uint8).  make them differents to get efficient  
	uint8 version;
	uint8 fastcode;  // xor value without this byte
	uint16 uid; // unique class id
	int32 size; // body size
	int32 reserved; // 64x alignment 

	FSNetBufferHead() :
		syncword(DEFAULT_SYNCWORD_INT32),
		version(0),
		fastcode(0),
		uid(0),
		size(0),
		reserved(0)
	{
	}

	FSNetBufferHead& operator=(const FSNetBufferHead& Other);
	FORCEINLINE bool MemRead(uint8 *Data, int32 BufferSize);
	FORCEINLINE static int32 MemSize();
};
#pragma pack(pop)


union UnionBufferHead
{
	uint8 byte[16]; // 4x 32bit
	FSNetBufferHead bufferHead;
};

union UnionSyncword
{
	uint8 byte[4]; // 4 char
	int32 value;
};

//0xE6B7F1A2
static UnionSyncword SyncwordDefault = { 230ui8, 183ui8, 241ui8, 162ui8 };

struct FSNetBufferBody
{
	uint8* bufferPtr;
	int32 length; // lenght == BufferHead.size;

	FSNetBufferBody()
		: bufferPtr(nullptr)
		, length(0)
	{}

	bool IsValid();
	FORCEINLINE bool MemRead(uint8 *Data, int32 BufferSize, int32 InLength);
	FORCEINLINE int32 MemSize();
};

/***************************************************/
/* 
	version 0: 
		only timestamp;
	version 1:
		[ signature, timestamp]
*/
/***************************************************/
#pragma pack(push, 1)
struct FSNetBufferFoot
{
// if use version, makesure #if SERVO_PROTOCOL_VERSION > 1 
#ifdef SERVO_PROTOCOL_SIGNATURE
	FSHA256Signature signature;
#endif // SERVO_PROTOCOL_SIGNATURE
	int64 timestamp; // unix timestamp

	FSNetBufferFoot()
		: timestamp(0)
	{
	}

	FSNetBufferFoot(int64 InTimestamp)
		: timestamp(InTimestamp)
	{
	}

	FORCEINLINE bool MemRead(uint8 *Data, int32 BufferSize);
	FORCEINLINE static int32 MemSize();
};
#pragma pack(pop)

/************************************************************/
/*
		Heartbeat:
		[head]
			uid == 0
			reserved == heart beat ext data
			size == 0
		[foot]
			(signature)
			timestamp
		[session]
*/
/************************************************************/
struct FSNetPacket
{
	FSNetBufferHead Head;
	FSNetBufferBody Body;
	FSNetBufferFoot Foot;

	// session id
	int32 sid;
	bool bFastIntegrity;

	bool IsValid();

	// check data integrity with fastcode
	static bool FastIntegrity(uint8* DataPtr, int32 DataLength, uint8 fastcode);

	FSNetPacket()
		: sid (0)
		, bFastIntegrity(false)
	{}

	FSNetPacket(uint8* Data, int32 BufferSize, int32& BytesRead, int32 InSyncword = DEFAULT_SYNCWORD_INT32);
};

/************************************************************/
/*
		Help
		TSharedPtr<FSNetPacket> packet = MakeShareable(FSNetPacket::Create(Data, BufferSize, BytesRead));
		TSharedPtr<FSNetPacket, ESPMode::ThreadSafe> packet = MakeShared<FSNetPacket, ESPMode::ThreadSafe>(?);
*/
/************************************************************/

/**
 * the protocol of SeptemServo
 */
class SEPTEMSERVO_API ServoProtocol
{
public:
	ServoProtocol();
	~ServoProtocol();
};
