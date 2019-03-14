// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ServoProtocol.h"

#include "SeptemAlgorithm/SeptemAlgorithm.h"
using namespace Septem;


#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformTime.h"
#elif  PLATFORM_LINUX
#include "Unix/UnixPlatformTime.h"
#endif

ServoProtocol::ServoProtocol()
{
}

ServoProtocol::~ServoProtocol()
{
}

bool FSNetBufferBody::IsValid()
{
	return bufferPtr != nullptr;
}

bool FSNetBufferBody::MemRead(uint8 * Data, int32 BufferSize, int32 InLength)
{
	if( BufferSize < InLength || InLength < 0)
		return false;

	length = InLength;
	bufferPtr = new uint8[length];
	
	FMemory::Memcpy(bufferPtr, Data, length);

	return true;
}

int32 FSNetBufferBody::MemSize()
{
	return length;
}

uint8 FSNetBufferBody::XOR()
{
	uint8 ret = 0;
	for (int32 i = 0; i < length; ++i)
		ret ^= bufferPtr[i];
	return ret;
}

FSNetBufferHead & FSNetBufferHead::operator=(const FSNetBufferHead & Other)
{
	FMemory::Memcpy(this, &Other, sizeof(FSNetBufferHead));
	return *this;
}

bool FSNetBufferHead::MemRead(uint8 * Data, int32 BufferSize)
{
	const int32 ReadSize = sizeof(FSNetBufferHead);
	if( BufferSize < ReadSize)
		return false;

	FMemory::Memcpy(this, Data, ReadSize);

	return true;
}

int32 FSNetBufferHead::MemSize()
{
	return sizeof(FSNetBufferHead);
}

uint8 FSNetBufferHead::XOR()
{
	uint8 ret = 0;
	uint8 *ptr = (uint8*)this;
	const int32 imax = sizeof(FSNetBufferHead);
	for (int32 i = 0; i < imax; ++i)
	{
		ret ^= ptr[i];
	}
	return ret;
}

bool FSNetPacket::IsValid()
{
	return bFastIntegrity;
}

bool FSNetPacket::FastIntegrity(uint8 * DataPtr, int32 DataLength, uint8 fastcode)
{
	uint8 xor = 0;
	for (int32 i = 0; i < DataLength; ++i)
	{
		xor ^= DataPtr[i];
	}
	return xor == fastcode;
}

FSNetPacket::FSNetPacket(uint8 * Data, int32 BufferSize, int32 & BytesRead, int32 InSyncword)
	:sid(0), bFastIntegrity(false)
{
	Head.syncword = InSyncword;
	// 1. find syncword for head
	int32 index = BufferBufferSyncword(Data, BufferSize, Head.syncword);
	uint8 fastcode = 0;

	if (-1 == index)
	{
		// failed to find syncword in the whole buffer
		BytesRead = BufferSize;
		return;
	}

	// 2. read head
	if (!Head.MemRead(Data + index, BufferSize - index))
	{
		// failed to read from the rest buffer
		BytesRead = BufferSize;
		return;
	}

	index += FSNetBufferHead::MemSize();
	fastcode ^= Head.XOR();

	if (0 == Head.uid)
	{
		Foot.timestamp = FPlatformTime::Cycles64();

		bFastIntegrity = fastcode == 0;
		BytesRead = index;
		return;
	}

	// 3. check and read body

	if (!Body.MemRead(Data + index, BufferSize - index, Head.size))
	{
		// failed to read from the rest buffer
		BytesRead = BufferSize;
		return;
	}
	index += Body.MemSize();
	fastcode ^= Body.XOR();

	// 4. read foot
	if (!Foot.MemRead(Data + index, BufferSize - index))
	{
		// failed to read from the rest buffer
		BytesRead = BufferSize;
		return;
	}

	index += FSNetBufferFoot::MemSize();
	fastcode ^= Foot.XOR();

	BytesRead = index;
	bFastIntegrity = fastcode == 0;

	return;
}

uint64 FSNetPacket::GetTimestamp()
{
	return Foot.timestamp;
}

FSNetPacket * FSNetPacket::CreateHeartbeat(int32 InSyncword)
{
	FSNetPacket* packet = new FSNetPacket();
	packet->Head.syncword = InSyncword;
	packet->Head.reserved = FPlatformTime::Cycles();
	packet->Head.fastcode = packet->Head.XOR();
	return nullptr;
}

bool FSNetBufferFoot::MemRead(uint8 * Data, int32 BufferSize)
{
	const int32 ReadSize = sizeof(FSNetBufferFoot);
	if (BufferSize < ReadSize)
		return false;

	FMemory::Memcpy(this, Data, ReadSize);

	return true;
}

int32 FSNetBufferFoot::MemSize()
{
	return sizeof(FSNetBufferFoot);
}

uint8 FSNetBufferFoot::XOR()
{
	uint8 ret = 0;
	uint8 *ptr = (uint8*)this;
	const int32 imax = sizeof(FSNetBufferFoot);
	for (int32 i = 0; i < imax; ++i)
	{
		ret ^= ptr[i];
	}
	return ret;
}
