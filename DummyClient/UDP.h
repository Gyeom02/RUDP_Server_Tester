#pragma once
#include "UDPSocket.h"
#include "UDPJob.h"

using UDPSocketPtr = shared_ptr<UDPSocket>;


class UDP
{

public:
	static enum
	{
		SOCKNUM = 1
	};
	UDP();
	virtual ~UDP() { UDPClear(); }


	bool UDPInit();
	void UDPClear();

	void CheckPacketPriority(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len);
	void UDPPacketHandle(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len);
	void UDPDoJop();
	UDPSocketPtr GetUDPSocket(int32 index)
	{
		return _udpSockets[index];
	}

	bool IsUDPOn() {
		return _IsUDPOn;
	}
	void SetIsOn(bool is)
	{
		_IsUDPOn = is;
	}
private:
	bool UDPSocketReset(int32 index);
private:
	Array<UDPSocketPtr, SOCKNUM> _udpSockets = {};
	bool _IsUDPOn = false;
};

extern UDP GUDP;

