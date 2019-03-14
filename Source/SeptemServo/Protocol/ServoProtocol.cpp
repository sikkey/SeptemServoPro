// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ServoProtocol.h"

#include "SeptemAlgorithm/SeptemAlgorithm.h"
using namespace Septem;

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
{
	bFastIntegrity = false;
	Head.syncword = InSyncword;
	// 1. find syncword for head
	int32 index = BufferBufferSyncword(Data, BufferSize, Head.syncword);

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

	// 3. check and read body

	if (!Body.MemRead(Data + index, BufferSize - index, Head.size))
	{
		// failed to read from the rest buffer
		BytesRead = BufferSize;
		return;
	}
	index += Body.MemSize();

	// 4. read foot
	if (!Foot.MemRead(Data + index, BufferSize - index))
	{
		// failed to read from the rest buffer
		BytesRead = BufferSize;
		return;
	}

	index += FSNetBufferFoot::MemSize();

	BytesRead = index;
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
