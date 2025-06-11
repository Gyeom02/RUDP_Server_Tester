#pragma once

#include "InFlightPacket.h"

class AckRange
{
public:
	AckRange() {}
	AckRange(int32 start) : mStart(start), mCount(1) {}
	~AckRange() {}

	AckRange operator=(const AckRange& ackRange) = delete;
	bool ExtendIfShould(PacketSequenceNumber SN);

	void AckWrite(OUT int32& start, OUT int32& count, OUT bool& hasCount);
	void AckRead(int32 start, int32 count);

	int32 GetStart() { return mStart; }
	int32 GetCount() { return mCount; }

	//NetAddress netAddress;
private:
	
	int32 mStart;
	int32 mCount;
};

class DeliveryNotificationManager : public enable_shared_from_this<DeliveryNotificationManager>
{
public:
	enum : ULONGLONG
	{
		TIMEOUT = 3000,
	};
	DeliveryNotificationManager();
	~DeliveryNotificationManager();

	//송신
	InFlightPacketPtr WriteSeqeuenceNumber(SOCKET object, NetAddress netAddr, SendBufferRef sendBuffer);
	void ProcessAcks(int32 start, int32 count, bool hasCount);
	void HandlePacketDeliveryFailure(const InFlightPacketPtr& inFlightPacket);
	void HandlePacketDeliverySuccess(const InFlightPacketPtr& inFlightPacket);


	//수신
	bool ProcessSequenceNumber(PacketSequenceNumber SN);
	void AddPendingAck(PacketSequenceNumber SN);
	bool WritePendingAcks(OUT int32& start, OUT int32& count, OUT bool& hasCount);

	//타임아웃체크
	void ProcessTimeOutPackets();

	int32 GetDeliveredPacketCount() { return mDeliveredPacketCount.load(); }
	int32 GetDroppedPacketCount() { return mDroppedPacketCount.load(); }
	int32 GetTimeOutCount() { return mTimeOutCount.load(); }
	int32 GetSequenceNotMatchedCount() { return mSequenceNotMatchedCount.load(); }
	uint32 GetDispatchedPacketCount() { return mDispatchedPacketCount.load(); }

	int32 GetSuccessReSendPacketNum() { return mSuccessReSendPacketNum.load(); }
	atomic<int32> mSuccessReSendPacketNum = 0;
private:
	//송신
	atomic<int32> mNextOutgoingSequenceNumber = 0; // 현재 송신된 패킷의 번호를 알려주는 변수
	atomic<int32> mDroppedPacketCount = 0;
	atomic<int32> mDeliveredPacketCount = 0;
	//수신
	atomic<int32> mNextExpectedSequenceNumber = 0; // 현재 수신된 패킷의 예상되는 세퀀스번호
	Deque<AckRange> mPendingAcks;
	//전체
	atomic<uint64> mDispatchedPacketCount; //전체 송신한 현재 패킷의 수 ( mDroppedPacketCount + mDeliveredPacketCount의 수와 같아야함 아님 패킷이 누락된거임)
	Deque<InFlightPacketPtr> mInFlightPackets;

	atomic<int32> mTimeOutCount = 0;
	atomic<int32> mSequenceNotMatchedCount = 0;
	
	USE_LOCK;
};
