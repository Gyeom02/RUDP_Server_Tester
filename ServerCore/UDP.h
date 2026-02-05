#pragma once
#include "UDPSocket.h"
#include "UDPRecvHandler.h"

using UDPSocketPtr = shared_ptr<UDPSocket>;


class UDP
{

public:
	static enum
	{
		SOCKNUM = 20,
		PLUS_WORKER_NUM = 0,
		//POPRECV_WORKER_NUM = 2,
		MAX_WORKER_NUM = QOS_SHARD_COUNT + PLUS_WORKER_NUM,
	};
	enum Type
	{
		SERVER = 1,
		CLIENT = 2,
	};
	UDP();
	virtual ~UDP() { UDPClear(); }


	bool UDPInit(Type type);
	void UDPClear();

	//void CheckPacketChannel(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len);
	//void UDPPacketHandle(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len);
	void UDPDo_PopRecv_Work(int32 start_index, int32 end_index);

	void InitRecvLogicWorkers(PacketHandleFunc func); //QoSCore의 Shard 수의 N /2 만큼 생성
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

	PacketHandleFunc _func; // 임시
private:
	bool UDPSocketReset(int32 index, PCWSTR serverAddr = L"");
private:
	Array<UDPSocketPtr, SOCKNUM> _udpSockets = {};
	Array<UDPRecvHandlerRef, MAX_WORKER_NUM> _udpRecvWorkers = {};
	bool _IsUDPOn = false;
	Type _type;
};

extern UDP GUDP;

