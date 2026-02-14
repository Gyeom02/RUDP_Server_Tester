#pragma once

//TimerWheel을 통한 Transport 층의 Timer TIck Event 실행 및 고유 JobQueue를 통한 Lock없는 단일 쓰레드 처리로 Lock 경쟁 제거

//class ControlJobs
//{
//public:
//
//	void PushJob(CallbackType&& callback);
//	bool Execute();
//	bool IsEmpty() {  return _jobs.Empty(); }
//	
//	SPSC::Queue<JobRef> _jobs;
//
//};


//#pragma pack(push, 1)

struct Ack
{
	uint8 bexsist = 0;
	uint8 bhascount = 0;
	uint32 start = 0;
	int32 count = -1;
	uint32 curExpectedSN = 0; // 현재 기다리고있는 패킷의 sn (RO Packet) 이 변수의 의미는 이보다 작은 SN의 패킷은 이미 받아서 처리했음을 의미
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
	uint64 sent_timestamp;
};

struct Ping
{
	uint8 bexsist = 0;
};

struct ControlHeader
{
	uint8 Type; // Control Type
	Ack ack;
	Rwind rwind; 
	Rtt rtt;
	Ping ping;
};
//#pragma pack(pop)

struct Packet_RTO_State
{
	InFlightPacketPtr inflightPacket;
	double rto_us = 0; // microseconds // UTime::now() + rto

	
};
struct RTO_Cmp
{
	bool operator()(const Packet_RTO_State& left, const Packet_RTO_State& right) { return left.rto_us < right.rto_us; }
};
class TransportControl
{
public:
	enum ControlType
	{
		ACK = 1,
		RECOVER_RWIND = 2,
		AD_RWIND = 3,
		RTT = 4,
		PING = 5,
	};
private:
	enum
	{
		ControlPacketSize = sizeof(PacketHeader) + sizeof(ControlHeader),
		TICKMS = 1,
		ACKTOKEN = 5,
		CONTROLJOB_TOKEN = 5,
		AD_RWIND_PERIOD = 200, // ms단위
	};
public:
	friend TickConnectManager;
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

	//void PushJob(CallbackType&& callback) { _controlJobWorker.PushJob(std::move(callback)); _lazyAssist._jobCv.notify_one(); }



public:
	static SendBufferRef MakeAckControlPacket(int32 client_id, int32 bhascount, uint32 start, int32 count, uint32 curExpectedSN, uint64 rtt_timestamp);
	static SendBufferRef MakeRecoverRwindControlPacket(int32 client_id, int32 rwindsize, uint32 total_recovered_size);
	static SendBufferRef MakeADRwindControlPacket(int32 client_id, int32 rwindsize, uint32 total_recovered_size);
	static SendBufferRef MakePingControlPacket(int32 client_id);
	//JobQueue Push Job

	void OnPushRWind(int32 client_id, int32 add_size, uint32 total_recovered_size);
	void OnSendPing(int32 client_id);//KeepAlive Ping Packet
	void OnPeriodicRwindSync(int32 client_id); // 송신측과 수신측의 Rwind 동기화를 위한 주기적인 Control Packet 송신

	/*----------------------*/

	bool EmptyReadyAckQueue();
	void PushHostAckReady(HostRef host);
	HostRef PopHostAckReady();

	bool EmptyReadyControlJobQueue();
	void PushHostControlJobReady(HostRef host);
	HostRef PopHostControlJobReady();

	LazyWorkAssist& GetLazyAssist() { return _lazyAssist; }

	void PushRTOQueue(const Packet_RTO_State& rhs);
	void PopRTOQueue();

	UTime::TimeWheel& GetTickTimerWheel() { return _tickTimeWheel; }
private:
	bool IsControlPacket(int16 flag) { if (flag <= 0) return false; else return true; }

	void HandleControlPacket(PacketHeader* header);

	void DoWork();  // Only One Thread Has to Run this Work Function(SPSC Queue Using)
//	void DoJobWork();
	static SendBufferRef MakeControlPacketBuffer(int32 client_id);
	static ControlHeader* InitControlHeader(BYTE* buffer);

	void HandleHostReadyAck(HostRef host);
	void HandleHostReadyControlJob(HostRef host);

	
	void CheckTimeOutPacket();
private:
	atomic<bool> brunning = false;
	
	
	LazyWorkAssist _lazyAssist;
	
	

	queue<weak_ptr<Host>> _readyAckHostQueue;
	queue<weak_ptr<Host>> _readyControlJobHostQueue;
	
	priority_queue<Packet_RTO_State, vector< Packet_RTO_State>, RTO_Cmp> _rtoMinQueue;

	UTime::TimeWheel _tickTimeWheel;

	USE_MANY_LOCKS(3); // For ReadyAckHostQueue, _readyControlJobHostQueue, _rtoMinQueue;

};

extern TransportControl GTransportControl;

