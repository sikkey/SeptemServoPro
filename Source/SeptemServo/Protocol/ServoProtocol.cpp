// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ServoProtocol.h"

#include "SeptemAlgorithm/SeptemAlgorithm.h"
using namespace Septem;

#include "Misc/ScopeLock.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformTime.h"
#elif  PLATFORM_LINUX
#include "Unix/UnixPlatformTime.h"
#endif

bool FSNetBufferBody::IsValid()
{
	return bufferPtr != nullptr;
}

bool FSNetBufferBody::MemRead(uint8 * Data, int32 BufferSize, int32 InLength)
{
	if( BufferSize < InLength || InLength < 0)
		return false;

	if (length > 0)
	{
		Reset();
	}

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

void FSNetBufferBody::Reset()
{
	if (bufferPtr)
	{
		delete bufferPtr;
		bufferPtr = nullptr;
		length = 0;
	}
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

void FSNetBufferHead::Reset()
{
	version = 0;
	fastcode = 0;
	uid = 0;
	size = 0;
	reserved = 0;
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
		// it is a heart beat packet
		Foot.timestamp = FPlatformTime::Cycles64();

		bFastIntegrity = fastcode == 0;
		sid = Head.reserved;
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

	sid = Head.reserved & ((1 << 30) - 1);

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

void FSNetPacket::ReUse(uint8 * Data, int32 BufferSize, int32 & BytesRead, int32 InSyncword)
{
	sid = 0;
	bFastIntegrity = false;

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
		// it is a heart beat packet
		Foot.timestamp = FPlatformTime::Cycles64();

		bFastIntegrity = fastcode == 0;
		sid = Head.reserved;
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

	sid = Head.reserved & ((1 << 30) - 1);

	return;
}

void FSNetPacket::OnDealloc()
{
	sid = 0;
	bFastIntegrity = false;
	Body.Reset();
	Head.size = 0;
}

void FSNetPacket::OnAlloc()
{
	Head.Reset();
	Foot.Reset();
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

FServoProtocol::FServoProtocol()
	:RecyclePool(RecyclePoolMaxnum)
{
	check(pSingleton == nullptr && "Protocol singleton can't create 2 object!");
	pSingleton = this;
	PacketPool = new TNetPacketQueue<FSNetPacket>();
}

FServoProtocol::~FServoProtocol()
{
	pSingleton = nullptr;
}

FServoProtocol * FServoProtocol::Get()
{
	if (nullptr == pSingleton) {
		FScopeLock lockSingleton(&mCriticalSection);
		pSingleton = new FServoProtocol();
	}
	
	return pSingleton;
}

FServoProtocol & FServoProtocol::GetRef()
{
	if (nullptr == pSingleton) {
		FScopeLock lockSingleton(&mCriticalSection);
		pSingleton = new FServoProtocol();
	}

	return *pSingleton;
}

FServoProtocol * FServoProtocol::Singleton()
{
	check(pSingleton && "Protocol singleton doesn't exist!");
	return pSingleton;
}

FServoProtocol & FServoProtocol::SingletonRef()
{
	check(pSingleton && "Protocol singleton doesn't exist!");
	return *pSingleton;
}

bool FServoProtocol::Push(TSharedPtr<FSNetPacket> InNetPacket)
{
	return PacketPool->Push(InNetPacket);
}

bool FServoProtocol::Pop(TSharedPtr<FSNetPacket>& OutNetPacket)
{
	return PacketPool->Pop(OutNetPacket);
}

TSharedPtr<FSNetPacket, ESPMode::ThreadSafe> FServoProtocol::AllocNetPacket()
{
	return RecyclePool.Alloc();
}

void FServoProtocol::DeallockNetPacket(const TSharedPtr<FSNetPacket, ESPMode::ThreadSafe>& InSharedPtr, bool bForceRecycle)
{
	InSharedPtr->OnDealloc();

	if (bForceRecycle)
	{
		RecyclePool.DeallocForceRecycle(InSharedPtr);
	}
	else 
	{
		RecyclePool.Dealloc(InSharedPtr);
	}
}

FServoProtocol* FServoProtocol::pSingleton = nullptr;
FCriticalSection FServoProtocol::mCriticalSection;
int32 FServoProtocol::RecyclePoolMaxnum = 1024;