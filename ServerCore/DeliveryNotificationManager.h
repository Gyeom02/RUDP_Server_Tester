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

	void AckWrite(OUT uint32& start, OUT int32& count, OUT bool& hasCount);
	void AckRead(uint32 start, int32 count);

	uint32 GetStart() { return mStart; }
	uint32 GetCount() { return mCount; }

	//NetAddress netAddress;
private:
	
	uint32 mStart;
	int32 mCount;
};

class DeliveryNotificationManager final: public enable_shared_from_this<DeliveryNotificationManager>
{
private:
	enum
	{
		InFlightPacket_LOCK = 0,
		Map_LOCK = 1, // For mSnToInFlightPacketMap
		Ack_LOCK = 2, // For mPendingAcks
	};
public:
	enum : ULONGLONG
	{
		TIMEOUT = 3000,
	};
	explicit DeliveryNotificationManager();
	~DeliveryNotificationManager();

	void SetOwner(shared_ptr<Host> owner) { _weakOwner = owner; }
	//Public
	// 
	bool CheckPacketChannel(PacketHeader* header);
	//////
	//송신
//	InFlightPacketPtr WriteSeqeuenceNumber(SendBufferRef sendBuffer);
	InFlightPacketPtr WriteSeqeuenceNumber(SendBufferRef sendBuffer);

	void ProcessAcks(uint32 start, uint32 count, bool hasCount, uint32 cuExpectedSN);
	void HandlePacketDeliveryFailure(const InFlightPacketPtr& inFlightPacket);
	void HandlePacketDeliverySuccess(const InFlightPacketPtr& inFlightPacket);


	//수신
	bool ProcessSequenceNumber(PacketHeader* header);
	void AddPendingAck(PacketSequenceNumber SN);
	bool WritePendingAcks(OUT uint32& start, OUT int32& count, OUT bool& hasCount);

	//타임아웃체크
	void ProcessTimeOutPackets();

	uint32 GetDeliveredPacketCount() { return mDeliveredPacketCount.load(); }
	uint32 GetDroppedPacketCount() { return mResendPacketCount.load() - mSuccessReSendPacketNum.load(); }
	uint32 GetResendPacketCount() { return mResendPacketCount.load(); }
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
	uint32 GetTotal_Recv_RWind() { return _recvWindow.GetTotalRWind(); }
	void MakeSpaceRWind(int32 doneSize) { _recvWindow.MakeSpaceRWind(doneSize); }
	void AddTotal_Recv_RWind(int32 doneSize) { _recvWindow.AddTotalRWind(doneSize); }

	/*   _sendWindow의 함수   */
	bool IsSpaceExistToSend(int32 sendSize) { return _sendWindow.IsSpaceExistToSend(sendSize); }
	void AddReceiverRWind(int32 rwindSize) { _sendWindow.AddReceiverRWind(rwindSize); }
	uint32 GetTotal_Send_RWind() { return _sendWindow.GetTotalRWind(); }
	void SetTotal_Send_RWind(int32 doneSize) { _sendWindow.SetTotalRWind(doneSize); }
	int32 GetReceiverRWind() { return _sendWindow.GetReceiverRWind(); }
	void StoreReceiverRWind(int32 rwindsize) { _sendWindow.StoreReceiverRWind(rwindsize); }


	bool GetbInsertAckReadyQueue() { return bInsertAckReadyQueue.load(); }
	void SetbInsertAckReadyQueue(bool to) { bInsertAckReadyQueue.exchange(to); }
	bool CheckHostAckEmpty();
private:
	InFlightPacketPtr FindInFlightPacketFromSN(uint32 sn);
	void StoreInFlightPacketFromSN(uint32 sn, InFlightPacketPtr inflightPacket);
	InFlightPacketPtr EraseInFlightPacketFronSN(uint32 sn);
	bool CheckValidAckSN(uint32 sn);
	InFlightPacketPtr HandleAck(uint32 sn);

	void UpdateExpectedAckSN(uint32 newSN);
private:
	/*   Reliable Ordered Packet의 변수   */
	//송신
	atomic<uint32> mNextOutgoingSequenceNumber = 0; // 현재 송신된 패킷의 번호를 알려주는 변수
	atomic<uint32> mResendPacketCount = 0;
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

	std::weak_ptr<Host> _weakOwner;

	unordered_map<uint32, InFlightPacketPtr> mSnToInFlightPacketMap; // sn % RUDPWIND::SN_RANGE_HALF to InFlightPacketPtr
	atomic<uint32> _curExpectedAckSN = 0;
	USE_MANY_LOCKS(3); // 0 = InPlightPacket , 1 = mSnToInFlightPacketMap , 2 = mPendingAcks
};
