// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "ServoProtocol.h"

#include "../SeptemAlgorithm/SeptemAlgorithm.h"
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
		
	}

	length = 0;
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

int32 FSNetBufferHead::SessionID()
{
	return reserved & ((1 << 22) - 1);
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

bool FSNetPacket::CheckIntegrity()
{
	if (Head.uid == 0)
	{
		return bFastIntegrity = (Head.XOR() ^ Foot.XOR()) == Head.fastcode;
	}
	return bFastIntegrity = (Head.XOR() ^ Body.XOR() ^ Foot.XOR()) == Head.fastcode;
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
		sid = Head.SessionID();
	}

	// 3. check and read body

	if (0 != Head.uid)
	{
		// it is not a heart beat packet
		if (!Body.MemRead(Data + index, BufferSize - index, Head.size))
		{
			// failed to read from the rest buffer
			BytesRead = BufferSize;
			return;
		}
		index += Body.MemSize();
		fastcode ^= Body.XOR();
	}

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
	bFastIntegrity = 0 == fastcode;

	sid = Head.SessionID();

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
	//packet->Head.reserved = FPlatformTime::Cycles();
	//packet->Foot.timestamp = FPlatformTime::Cycles64();
	//double now = FPlatformTime::Seconds();
	//packet->Foot.timestamp = FDateTime::UtcNow().ToUnixTimestamp();
	packet->Foot.SetNow();

	// No need body xor
	packet->Head.fastcode = packet->Head.XOR() ^ packet->Foot.XOR();
	return packet;
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
		sid = Head.SessionID();
	}

	// 3. check and read body
	if (0 != Head.uid)
	{
		if (!Body.MemRead(Data + index, BufferSize - index, Head.size))
		{
			// failed to read from the rest buffer
			BytesRead = BufferSize;
			return;
		}
		index += Body.MemSize();
		fastcode ^= Body.XOR();
	}

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

	sid = Head.SessionID();

	return;
}

void FSNetPacket::WriteToArray(TArray<uint8>& InBufferArr)
{
	int32 BytesWrite = 0;
	int32 writeSize = sizeof(FSNetBufferHead) + sizeof(FSNetBufferFoot);
	if (Head.uid > 0ui16)
	{
		writeSize += Body.length;
	}

	InBufferArr.SetNumZeroed (writeSize);
	uint8* DataPtr = InBufferArr.GetData();
	
	//1. write heads
	FMemory::Memcpy(DataPtr, &Head, FSNetBufferHead::MemSize());
	BytesWrite += sizeof(FSNetBufferHead);
	
	//2. write Body
	if (Head.uid != 0)
	{
		FMemory::Memcpy(DataPtr + BytesWrite, Body.bufferPtr, Body.length);
		BytesWrite += Body.length;
	}

	//3. write Foot
	FMemory::Memcpy(DataPtr + BytesWrite, &Foot, FSNetBufferFoot::MemSize());
	BytesWrite += FSNetBufferFoot::MemSize();
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

void FSNetPacket::ReUseAsHeartbeat(int32 InSyncword)
{
	Head.Reset();
	Head.syncword = InSyncword;
	Body.Reset();
	Foot.SetNow();
	Head.fastcode = Head.XOR() ^ Foot.XOR();
}

bool FSNetPacket::operator<(const FSNetPacket & Other)
{
	return Foot.timestamp < Other.Foot.timestamp;
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

void FSNetBufferFoot::SetNow()
{
#ifdef SERVO_PROTOCOL_SIGNATURE
	memset(signature, 0, sizeof(signature));
#endif // SERVO_PROTOCOL_SIGNATURE
	//timestamp = FPlatformTime::Cycles64();
	//double now = FPlatformTime::Seconds();
	timestamp = Septem::UnixTimestampMillisecond(); //(FDateTime::UtcNow().GetTicks() - FDateTime(1970, 1, 1).GetTicks())/ETimespan::TicksPerMillisecond;
}

FServoProtocol::FServoProtocol()
	:Syncword(DEFAULT_SYNCWORD_INT32)
	, PacketPoolCount(0)
	, RecyclePool(RecyclePoolMaxnum)
{
	check(pSingleton == nullptr && "Protocol singleton can't create 2 object!");
	pSingleton = this;
	PacketPool = new TNetPacketQueue<FSNetPacket, ESPMode::ThreadSafe>();
}

FServoProtocol::~FServoProtocol()
{
	pSingleton = nullptr;
	delete PacketPool;
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

bool FServoProtocol::Push(const TSharedPtr<FSNetPacket, ESPMode::ThreadSafe>& InNetPacket)
{
	if (PacketPool->Push(InNetPacket))
	{
		++PacketPoolCount;
		return true;
	}

	return false;
}

bool FServoProtocol::Pop(TSharedPtr<FSNetPacket, ESPMode::ThreadSafe>& OutNetPacket)
{
	if (PacketPool->Pop(OutNetPacket))
	{
		--PacketPoolCount;
		return true;
	}

	return false;
}

int32 FServoProtocol::PacketPoolNum()
{
	return PacketPoolCount;
}

TSharedPtr<FSNetPacket, ESPMode::ThreadSafe> FServoProtocol::AllocNetPacket()
{
	return RecyclePool.Alloc();
}

TSharedPtr<FSNetPacket, ESPMode::ThreadSafe> FServoProtocol::AllocHeartbeat()
{
	TSharedPtr<FSNetPacket, ESPMode::ThreadSafe>&& ret = RecyclePool.Alloc();

	ret->ReUseAsHeartbeat(Syncword);

	ret->CheckIntegrity();

	return ret;
}

void FServoProtocol::DeallockNetPacket(const TSharedPtr<FSNetPacket, ESPMode::ThreadSafe>& InSharedPtr, bool bForceRecycle)
{
	if (!InSharedPtr.IsValid())
		return;

	//UE_LOG(LogTemp, Display, TEXT("OnDeallocBegin"));
	InSharedPtr->OnDealloc();
	//UE_LOG(LogTemp, Display, TEXT("OnDeallocEnd"));
	if (bForceRecycle)
	{
		RecyclePool.DeallocForceRecycle(InSharedPtr);
	}
	else 
	{
		//UE_LOG(LogTemp, Display, TEXT("recycle begin"));
		RecyclePool.Dealloc(InSharedPtr);
		//UE_LOG(LogTemp, Display, TEXT("recycle end"));
	}
}

int32 FServoProtocol::RecyclePoolNum()
{
	return RecyclePool.Num();
}

bool FServoProtocol::PopWithRecycle(TSharedPtr<FSNetPacket, ESPMode::ThreadSafe>& OutRecyclePacket)
{
	TSharedPtr<FSNetPacket, ESPMode::ThreadSafe> newPacket;
	if (Pop(newPacket))
	{
		DeallockNetPacket(OutRecyclePacket);
		OutRecyclePacket = MoveTemp(newPacket);
		return true;
	}

	return false;
}

FServoProtocol* FServoProtocol::pSingleton = nullptr;
FCriticalSection FServoProtocol::mCriticalSection;
int32 FServoProtocol::RecyclePoolMaxnum = 1024;