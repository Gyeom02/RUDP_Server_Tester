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
	int32 Send(HostRef player, SendBufferRef sendBuffer);
	int32 ControlSend(HostRef player, SendBufferRef sendBuffer);
	int32 PriortySend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffers);

protected:
	bool CheckMSSover(SendBufferRef sendBuffer);

	//int32 NormalSend(HostRef player, SendBufferRef sendBuffer);
	int32 NormalSend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffer);
	int32 SizeOverSend(HostRef player, SendBufferRef sendBuffer);

	SendBufferRef MakeFragmentBuffer(int32 size);
	
private:
	int ReliableSend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffers);
	int UnReliable_Ordered_Send(HostRef player, SendBufferRef sendBuffer); // Only For ControlPacket
	int UnReliableSend(HostRef player, SendBufferRef sendBuffer);
	//int Pure_Send(HostRef player, SendBufferRef sendBuffer);

	int FPCSend(HostRef player, SendBufferRef sendBuffer); // Pure Straight Send Function
	int FPCSend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffer); // Pure Straight Send Function

	//int UnReliable_Control_Send(HostRef player, SendBufferRef sendBuffer);
//	int ReliableSend(HostRef player, SendBufferRef sendBuffer);
	//int UnReliable_Ordered_Send(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffer);
	//int UnReliableSend(HostRef player, SendBufferRef sendBuffer);
	
	SOCKET _socket = INVALID_SOCKET;
	WSAEVENT _wsaEvent;
	int32 _clientNum = 0;
	RecvBuffer udpRecvBuffer;
	NetAddress _serverAddress;

	USE_LOCK;
};

