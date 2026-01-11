#pragma once
#include "RecvBuffer.h"
//#include "Player
//using PlayerRef = shared_ptr<class Player>;

class UDPSocket : public enable_shared_from_this<UDPSocket>
{
	enum {
		CLMAX = 100,
		MSS = 1460,
		USER_MSS = MSS - sizeof(PacketHeader),
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
	int32 Send(ObjectRef player, SendBufferRef sendBuffer);
	int32 PriortySend(ObjectRef player, SendBufferRef sendBuffer);

protected:
	bool CheckMSSover(SendBufferRef sendBuffer);

	int32 NormalSend(ObjectRef player, SendBufferRef sendBuffer);
	int32 SizeOverSend(ObjectRef player, SendBufferRef sendBuffer);

	SendBufferRef MakeFragmentBuffer(int32 size);
	
private:
	int ReliableSend(ObjectRef player, SendBufferRef sendBuffer);
	int UnReliable_Ordered_Send(ObjectRef player, SendBufferRef sendBuffer);
	int UnReliableSend(ObjectRef player, SendBufferRef sendBuffer);
	int FPCSend(ObjectRef player, SendBufferRef sendBuffer); // Pure Straight Send Function
	SOCKET _socket = INVALID_SOCKET;
	WSAEVENT _wsaEvent;
	int32 _clientNum = 0;
	RecvBuffer udpRecvBuffer;
	NetAddress _serverAddress;
};

