#pragma once
#include "RecvBuffer.h"
//#include "Player
using PlayerRef = shared_ptr<class Player>;

class UDPSocket : public enable_shared_from_this<UDPSocket>
{
	enum {
		CLMAX = 100
	};
public:
	UDPSocket();
	~UDPSocket();

	void UDPWork();

	SOCKET& GetSocket()
	{
		return _socket;
	}
	NetAddress& GetNetAddress()
	{
		return _serverAddress;
	}
	void AddClient() {
		_clientNum++;
	}
	void ReleaseClient() {
		_clientNum--;
	}
	WSAEVENT& GetWSAEvent()
	{
		return _wsaEvent;
	}

public:
	int32 Send(PlayerRef player, SendBufferRef sendBuffer);
	int32 PriortySend(PlayerRef player, SendBufferRef sendBuffer);
private:
	int ReliableSend(PlayerRef player, SendBufferRef sendBuffer);
	int UnReliableSend(PlayerRef player, SendBufferRef sendBuffer);
	int FPCSend(PlayerRef player, SendBufferRef sendBuffer); // Pure Straight Send Function
	SOCKET _socket = INVALID_SOCKET;
	WSAEVENT _wsaEvent;
	int32 _clientNum = 0;
	RecvBuffer udpRecvBuffer;
	NetAddress _serverAddress;
};

