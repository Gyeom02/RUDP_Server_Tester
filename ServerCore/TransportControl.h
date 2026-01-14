#pragma once
#include "RUDPUtils.h"
//TimerWheel을 통한 Transport 층의 Timer TIck Event 실행 및 고유 JobQueue를 통한 Lock없는 단일 쓰레드 처리로 Lock 경쟁 제거

class ControlJobWorker
{
public:

	void PushJob(CallbackType&& callback);
	bool Execute();
	bool IsEmpty() { READ_LOCK; return _jobs.empty(); }
	
	queue<JobRef> _jobs;
	USE_LOCK;
};

struct Ack
{
	uint8 bexsist = 0;
	int32 bhascount = -1;
	int32 start = -1;
	int32 count = -1;
};
struct RecoverRWind //Recv를 통해 사용한 Rwind의 버퍼 사이즈 값을 QoS에서 Logic Layer로 올리기전 상대방에게 처리된 패킷의 사이즈 값을 다시보내 RWind 회복에 도움
{
	uint8 bexsist = 0;
	int32 rwindsize = -1;
};
struct ADRWind //UsedRWind와 개별적으로 현재 RWind의 전체 사이즈를 보냄, 패킷 진행도(패킷을 보낸 수와 받은 수를 매치하여 상대방은 해당 패킷을 무시하거나 ReceiverWind를 업데이트함
{
	uint8 bexsist = 0;
	int32 rwindsize = -1;
	uint32 packetHandleCount = -1;
};
struct Rtt
{
	uint8 bexsist = 0;
	LONGLONG sent_timestamp;
};
struct ControlHeader
{
	int16 Type; // Server Or Client

	Ack ack;
	RecoverRWind rwind;
	Rtt rtt;
};

class TransportControl
{
private:
	enum
	{
		ControlPacketSize = sizeof(PacketHeader) + sizeof(ControlHeader),
		TICKMS = LazyAssist::NORMAL_TickMs,
	};
public:
	/*enum ControlType
	{
		INIT = 1,
		ACK = 2,
		RWIND = 3,
	};*/
public:

	TransportControl(uint32 tickms = TICKMS);
	~TransportControl();
	bool CheckValidControl(PacketHeader* header);

	void PushJob(CallbackType&& callback) { _jobWorker.PushJob(std::move(callback)); _lazyAssist._jobCv.notify_one(); }

	void RunThread();
	void StopThread();

public:
	void OnPushRWind(int32 client_id, int32 add_size);
private:
	bool IsControlPacket(int16 flag) { if (flag <= 0) return false; else return true; }

	void HandleControlPacket(PacketHeader* header);

	void DoWork();
//	void DoJobWork();
public:
	static SendBufferRef MakeAckControlPacket(int32 client_id, int32 bhascount,int32 start,int32 count);
	static SendBufferRef MakeRecoverRwindControlPacket(int32 client_id, int32 rwindsize);
	static SendBufferRef MakeADRwindControlPacket(int32 client_id, int32 rwindsize, uint32 packetHandleCount);
	
private:
	atomic<bool> brunning = false;
	ControlJobWorker _jobWorker;
	//LazyWorkAssist& GetLazyAssist() { return _lazyAssist; }
	LazyWorkAssist _lazyAssist;
	static SendBufferRef MakeControlPacketBuffer(int32 client_id);
	
};

extern TransportControl GTransportControl;

