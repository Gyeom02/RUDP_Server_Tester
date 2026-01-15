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
struct Rwind 
{
	uint8 bexsist = 0;
	int32 rwindsize = -1;
	uint32 total_recovered_size = -1;
};

struct Rtt
{
	uint8 bexsist = 0;
	LONGLONG sent_timestamp;
};
#pragma pack(push, 1)
struct ControlHeader
{
	uint8 Type; // Control Type
	Ack ack;
	Rwind rwind; 
	Rtt rtt;
};
#pragma pack(pop)

class TransportControl
{
public:
	enum ControlType
	{
		ACK = 1,
		RECOVER_RWIND = 2,
		AD_RWIND = 3,
		RTT = 4,
	};
private:
	enum
	{
		ControlPacketSize = sizeof(PacketHeader) + sizeof(ControlHeader),
		TICKMS = LazyAssist::NORMAL_TickMs,
		ACKTOKEN = 5,
		AD_RWIND_PERIOD = 200, // ms단위
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

	void RunThread();
	void StopThread();

	bool CheckValidControl(PacketHeader* header);

	void PushJob(CallbackType&& callback) { _jobWorker.PushJob(std::move(callback)); _lazyAssist._jobCv.notify_one(); }



public:
	static SendBufferRef MakeAckControlPacket(int32 client_id, int32 bhascount, int32 start, int32 count);
	static SendBufferRef MakeRecoverRwindControlPacket(int32 client_id, int32 rwindsize, uint32 total_recovered_size);
	static SendBufferRef MakeADRwindControlPacket(int32 client_id, int32 rwindsize, uint32 total_recovered_size);


	void OnPushRWind(int32 client_id, int32 add_size, uint32 total_recovered_size);

	bool EmptyReadyAckQueue();
	void PushHostAckReady(HostRef host);
	HostRef PopHostAckReady();

	LazyWorkAssist& GetLazyAssist() { return _lazyAssist; }
private:
	bool IsControlPacket(int16 flag) { if (flag <= 0) return false; else return true; }

	void HandleControlPacket(PacketHeader* header);

	void DoWork();
//	void DoJobWork();
	static SendBufferRef MakeControlPacketBuffer(int32 client_id);

	void HandleHostReadyAck(HostRef host);
	
	void PeriodicRwindSync(HostRef host, int32 rwindsize, uint32 total_recovered_size); // 송신측과 수신측의 Rwind 동기화를 위한 주기적인 Control Packet 송신
private:
	atomic<bool> brunning = false;
	ControlJobWorker _jobWorker;
	
	LazyWorkAssist _lazyAssist;
	

	queue<HostRef> _readyAckHostQueue;

	
	USE_LOCK; // For ReadyAckHostQueue;
};

extern TransportControl GTransportControl;

