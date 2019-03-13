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
		return;
	}

	// 2. read head
	Head.MemRead(Data + index, BufferSize - index);
	// TODO: 3. check and read body
	// TODO: 4. read foot

	BytesRead = index;
}
