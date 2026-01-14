#pragma once

//TimerWheel을 통한 Transport 층의 Timer TIck Event 실행 및 고유 JobQueue를 통한 Lock없는 단일 쓰레드 처리로 Lock 경쟁 제거

class ControlJobWorker
{
public:
	void PushJob(CallbackType&& callback);
	
	bool Execute();
private:
	
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
struct RWind
{
	uint8 bexsist = 0;
	int32 rwindsize = -1;
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
	RWind rwind;
	Rtt rtt;
};

class TransportControlPlane
{
private:
	enum
	{
		ControlPacketSize = sizeof(PacketHeader) + sizeof(ControlHeader),
	};
public:
	/*enum ControlType
	{
		INIT = 1,
		ACK = 2,
		RWIND = 3,
	};*/
public:

	TransportControlPlane();
	~TransportControlPlane();
	bool CheckValidControl(PacketHeader* header);

	void PushJob(CallbackType&& callback) { jobworker.PushJob(std::move(callback)); }

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
	static SendBufferRef MakeControlPacketBuffer(int32 client_id);
private:
	bool brunning = false;
	ControlJobWorker jobworker;
	
};

extern TransportControlPlane GTransportControl;

