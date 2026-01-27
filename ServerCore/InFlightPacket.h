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
	explicit InFlightPacket(std::weak_ptr<Host> owner, PacketSequenceNumber packetSN, SendBufferRef sendBuffer) : _owner(owner), _packetSequenceNumber(packetSN), _sendBuffer(sendBuffer) { mTimeDispatched = GetTickCount64(); }
	~InFlightPacket() {}

	shared_ptr<Host> GetOwner() { return _owner.lock(); }
	void SetTransmissionData(SendBufferRef sendBuffer) { _sendBuffer = sendBuffer; }
	SendBufferRef GetTransmissionData() { return _sendBuffer; }
	void HandleDeliveryFailure(DeliveryManagerRef deliveryManager);
	void HandleDeliverySuccess(DeliveryManagerRef deliveryManager);
	
	PacketSequenceNumber& GetSequenceNumber() { return _packetSequenceNumber; }
	ULONGLONG GetTimeDispactched() { return mTimeDispatched;  }

	//int32 Send();


private:
	
	PacketSequenceNumber _packetSequenceNumber;
	ULONGLONG mTimeDispatched = 0;
	SendBufferRef _sendBuffer;
	std::weak_ptr<Host> _owner;
	
};
using InFlightPacketPtr = shared_ptr<InFlightPacket>;
