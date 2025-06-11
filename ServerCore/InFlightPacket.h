#pragma once



class PacketSequenceNumber
{
public:
	PacketSequenceNumber(int32 n = -1) :_sequenceNumber(n) {}
	PacketSequenceNumber(const PacketSequenceNumber& psn) { _sequenceNumber = psn._sequenceNumber; }
	~PacketSequenceNumber() {}

	PacketSequenceNumber operator=(const PacketSequenceNumber& psn) { _sequenceNumber = psn._sequenceNumber; return *this; }
	
	bool operator < (const PacketSequenceNumber& otherSN) const { return _sequenceNumber < otherSN._sequenceNumber; }
	bool operator > (const PacketSequenceNumber& otherSN) const { return _sequenceNumber > otherSN._sequenceNumber; }
	bool operator == (const PacketSequenceNumber& otherSN) const { return _sequenceNumber == otherSN._sequenceNumber; }
	void operator ++() { ++_sequenceNumber; }


	int32 GetSN() { return _sequenceNumber; }
	void SetSN(int32 sn) { _sequenceNumber = sn; }

private:
	int32 _sequenceNumber = -1;
	
};

class InFlightPacket
{
public:
	InFlightPacket(SOCKET object, NetAddress netAddr, PacketSequenceNumber packetSN, SendBufferRef sendBuffer) : _socket(object), _netAddr(netAddr), _packetSequenceNumber(packetSN), _sendBuffer(sendBuffer) { mTimeDispatched = GetTickCount64(); }
	~InFlightPacket() {}

	NetAddress GetNetAddr() { return _netAddr; }
	SOCKET GetSocket() { return _socket; }

	void SetTransmissionData(SendBufferRef sendBuffer) { _sendBuffer = sendBuffer; }
	SendBufferRef GetTransmissionData() { return _sendBuffer; }
	void HandleDeliveryFailure(DeliveryManagerRef deliveryManager);
	void HandleDeliverySuccess(DeliveryManagerRef deliveryManager);
	
	PacketSequenceNumber& GetSequenceNumber() { return _packetSequenceNumber; }
	ULONGLONG GetTimeDispactched() { return mTimeDispatched;  }

	int32 Send();

private:
	PacketSequenceNumber _packetSequenceNumber;
	ULONGLONG mTimeDispatched = 0;
	SendBufferRef _sendBuffer;
	NetAddress _netAddr;
	SOCKET _socket;
};
using InFlightPacketPtr = shared_ptr<InFlightPacket>;
