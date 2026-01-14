#pragma once

#include "InFlightPacket.h"
#include "RUDPWindow.h"

class Host;

class AckRange
{
public:
	AckRange() {}
	AckRange(uint32 start) : mStart(start), mCount(1) {}
	~AckRange() {}

	AckRange operator=(const AckRange& ackRange) = delete;
	bool ExtendIfShould(PacketSequenceNumber SN);

	void AckWrite(OUT uint32& start, OUT uint32& count, OUT bool& hasCount);
	void AckRead(uint32 start, uint32 count);

	uint32 GetStart() { return mStart; }
	uint32 GetCount() { return mCount; }

	//NetAddress netAddress;
private:
	
	uint32 mStart;
	uint32 mCount;
};

class DeliveryNotificationManager : public enable_shared_from_this<DeliveryNotificationManager>
{
public:
	enum : ULONGLONG
	{
		TIMEOUT = 3000,
	};
	explicit DeliveryNotificationManager();
	~DeliveryNotificationManager();

	void SetOwner(shared_ptr<Host> owner) { _owner = owner; }
	//Public
	// 
	bool CheckPacketChannel(int16 channel, uint32 sn, int32 size);
	//////
	//송신
	InFlightPacketPtr WriteSeqeuenceNumber(SOCKET object, NetAddress netAddr, SendBufferRef sendBuffer);
	InFlightPacketPtr WriteSeqeuenceNumber(SOCKET object, NetAddress netAddr, shared_ptr<vector<SendBufferRef>>sendBuffer);

	void ProcessAcks(uint32 start, uint32 count, bool hasCount);
	void HandlePacketDeliveryFailure(const InFlightPacketPtr& inFlightPacket);
	void HandlePacketDeliverySuccess(const InFlightPacketPtr& inFlightPacket);


	//수신
	bool ProcessSequenceNumber(PacketSequenceNumber SN, int32 size);
	void AddPendingAck(PacketSequenceNumber SN);
	bool WritePendingAcks(OUT uint32& start, OUT uint32& count, OUT bool& hasCount);

	//타임아웃체크
	void ProcessTimeOutPackets();

	uint32 GetDeliveredPacketCount() { return mDeliveredPacketCount.load(); }
	uint32 GetDroppedPacketCount() { return mDroppedPacketCount.load(); }
	int32 GetTimeOutCount() { return mTimeOutCount.load(); }
	int32 GetSequenceNotMatchedCount() { return mSequenceNotMatchedCount.load(); }
	uint32 GetDispatchedPacketCount() { return mDispatchedPacketCount.load(); }

	uint32 GetSuccessReSendPacketNum() { return mSuccessReSendPacketNum.load(); }
	atomic<uint32> mSuccessReSendPacketNum = 0;


	/*--------------------------------------------*/
	/*   Unreliable Ordered Packet의 함수   */

	bool ProcessSequenceNumber_URO(PacketSequenceNumber SN);

	bool WriteSeqeuenceNumber_URO(SendBufferRef sendBuffer);
	bool WriteSeqeuenceNumber_URO(shared_ptr<vector<SendBufferRef>> sendBuffer);

	/*--------------------------------------------*/
	/*   _recvWindow의 함수   */
	int32 GetExpectedSeqNum() { return _recvWindow.GetExpectedSqeNum(); }
	int32 GetRWind() { return _recvWindow.GetRWind(); }
	void MakeSpaceRWind(int32 doneSize) { _recvWindow.MakeSpaceRWind(doneSize); }

	/*   _sendWindow의 함수   */
	bool IsSpaceExistToSend(int32 sendSize) { return _sendWindow.IsSpaceExistToSend(sendSize); }
	void AddReceiverRWind(int32 rwindSize) { _sendWindow.AddReceiverRWind(rwindSize); }
	int32 GetReceiverRWind() { return _sendWindow.GetReceiverRWind(); }

	bool GetbInsertAckReadyQueue() { return bInsertAckReadyQueue.load(); }
	void SetbInsertAckReadyQueue(bool to) { bInsertAckReadyQueue.exchange(to); }
	bool CheckHostAckEmpty();

private:
	/*   Reliable Ordered Packet의 변수   */
	//송신
	atomic<uint32> mNextOutgoingSequenceNumber = 0; // 현재 송신된 패킷의 번호를 알려주는 변수
	atomic<uint32> mDroppedPacketCount = 0;
	atomic<uint32> mDeliveredPacketCount = 0;
	//수신
	atomic<uint32> mNextExpectedSequenceNumber = 0; // 현재 수신된 패킷의 예상되는 세퀀스번호
	Deque<AckRange> mPendingAcks;
	//전체
	atomic<uint64> mDispatchedPacketCount; //전체 송신한 현재 패킷의 수 ( mDroppedPacketCount + mDeliveredPacketCount의 수와 같아야함 아님 패킷이 누락된거임)
	Deque<InFlightPacketPtr> mInFlightPackets;

	atomic<int32> mTimeOutCount = 0;
	atomic<int32> mSequenceNotMatchedCount = 0;

	/*    ---------------------------    */

	/*   Unreliable Ordered Packet의 변수   */
	atomic<uint32> mNextOutgoingSequenceNumber_URO = 0;
	atomic<uint32> mNextExpectedSequenceNumber_URO = 0;

	RUDPRecvWindow _recvWindow;
	RUDPSendWindow _sendWindow;

	atomic<bool> bInsertAckReadyQueue = false;

	std::weak_ptr<Host> _owner;
	USE_LOCK;
};
