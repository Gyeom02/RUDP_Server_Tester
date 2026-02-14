#pragma once

class Host;

class PacketSequenceNumber
{
public:
	PacketSequenceNumber(uint32 n = -1) :_sequenceNumber(n) {}
	PacketSequenceNumber(const PacketSequenceNumber& psn) { _sequenceNumber = psn._sequenceNumber; }
	~PacketSequenceNumber() {}

	PacketSequenceNumber operator=(const PacketSequenceNumber& psn) { _sequenceNumber = psn._sequenceNumber; return *this; }
	
	bool operator < (const PacketSequenceNumber& otherSN) const { return _sequenceNumber < otherSN._sequenceNumber; }
	bool operator > (const PacketSequenceNumber& otherSN) const { return _sequenceNumber > otherSN._sequenceNumber; }
	bool operator == (const PacketSequenceNumber& otherSN) const { return _sequenceNumber == otherSN._sequenceNumber; }
	void operator ++() { ++_sequenceNumber; }


	uint32 GetSN() { return _sequenceNumber; }
	void SetSN(uint32 sn) { _sequenceNumber = sn; }

private:
	uint32 _sequenceNumber = -1;
	
};

class InFlightPacket
{
public:
	explicit InFlightPacket(std::weak_ptr<Host> owner, PacketSequenceNumber packetSN, SendBufferRef sendBuffer) : _owner(owner), _packetSequenceNumber(packetSN), _sendBuffer(sendBuffer) {  }
	~InFlightPacket() {}

	shared_ptr<Host> GetOwner() { return _owner.lock(); }
	void SetTransmissionData(SendBufferRef sendBuffer) { _sendBuffer = sendBuffer; }
	SendBufferRef GetTransmissionData() { return _sendBuffer; }
	void HandleDeliveryFailure(DeliveryManagerRef deliveryManager); //Resend
	void HandleDeliverySuccess(DeliveryManagerRef deliveryManager);
	
	PacketSequenceNumber& GetSequenceNumber() { return _packetSequenceNumber; }
	//ULONGLONG GetTimeDispactched() { return mTimeDispatched;  }

	//int32 Send();

	void InitSendTime();
	uint64 GetRecentSendTime();
	void UpdateRecentSendTime();
	bool Check_CoolTime_ReSend();

public: // For RUDP
	bool IsGotAck() { return _bAcked.load(); }
	void GotAck() { SetbAcked(true); }
private:
	void SetbAcked(bool bAck) { _bAcked.store(bAck); }
	atomic<bool> _bAcked = false;

private:
	atomic<uint64> _time_first_send = 0;
	atomic<uint64> _time_recent_send = 0;
	atomic<uint64> _time_recent_resend = 0;
	PacketSequenceNumber _packetSequenceNumber;
	//ULONGLONG mTimeDispatched = 0;
	SendBufferRef _sendBuffer;
	std::weak_ptr<Host> _owner;
	
};
using InFlightPacketPtr = shared_ptr<InFlightPacket>;
