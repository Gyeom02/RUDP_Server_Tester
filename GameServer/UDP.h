#pragma once
#include "UDPSocket.h"
#include "UDPRecvHandler.h"

using UDPSocketPtr = shared_ptr<UDPSocket>;


class UDP
{

public:
	static enum
	{
		SOCKNUM = 1,
		PLUS_WORKER_NUM = 0,
		//POPRECV_WORKER_NUM = 2,
		MAX_WORKER_NUM = QOS_SHARD_COUNT + PLUS_WORKER_NUM,
	};
	UDP();
	virtual ~UDP() { UDPClear(); }


	bool UDPInit();
	void UDPClear();

	//void CheckPacketChannel(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len);
	//void UDPPacketHandle(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len);
	void UDPDo_PopRecv_Work(int32 start_index, int32 end_index);

	void InitRecvLogicWorkers(); //QoSCore의 Shard 수의 N /2 만큼 생성
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
	Array<UDPRecvHandlerRef, MAX_WORKER_NUM> _udpRecvWorkers = {};
	bool _IsUDPOn = false;
};

extern UDP GUDP;

