#pragma once
//Class For Quality of Service
#include "Nids.h"
#include <map>
#include "Host.h"
#include "Lock.h"
#include "FragmentManager.h"

#define RECV_WRITE_LOCK WRITE_LOCK_IDX(0)
#define SEND_WRITE_LOCK WRITE_LOCK_IDX(1)
#define RECV_READ_LOCK READ_LOCK_IDX(0)
#define SEND_READ_LOCK READ_LOCK_IDX(1)


constexpr const int32 QOS_SHARD_COUNT = 4;
using TimePoint = chrono::steady_clock::time_point;

struct QoSConfig
{
	int32 maxTokensPerSeocnd = 60;
	int32 burstLimit = 10;
};

class TokenBucket
{
	int32 tokens;
	int32 maxTokens;
	int32 refilRate;
	TimePoint lastRefil;

public:
	TokenBucket(int32 maxTokenSec, int32 burst) : maxTokens(burst), tokens{ burst }, refilRate(maxTokenSec), lastRefil(chrono::steady_clock::now()) {}

	void Refil();

	bool Consume();
};


struct SavedSendPacket
{
	shared_ptr<vector<SendBufferRef>> sendBuffers;
	int32 AllBuffersSize = 0;
	SavedSendPacket(shared_ptr<vector<SendBufferRef>> buffers) :sendBuffers(buffers) { for (int32 i = 0; i < (*buffers).size(); i++) AllBuffersSize += (*buffers)[i]->WriteSize(); }
	//SavedSendPacket(SendBufferRef buffer) { }
	// TODO : Network 층에서의 조각화를 사전에 막기위해 Linked형태로 다음 또는 이전의 패킷의 ptr을 갖고있어야함
};
struct SavedRecvPacket
{
	explicit SavedRecvPacket(BYTE* buffer, int32 size); // 그냥 쌩 new BYTE* Pointer 아님 큰일남,소멸자에서 해당 버퍼를 delete 처리하기 때문이다
	~SavedRecvPacket();


	BYTE* _buffer;
	int32 _size;

	// TODO : Network 층에서의 조각화를 사전에 막기위해 Linked형태로 다음 또는 이전의 패킷의 ptr을 갖고있어야함
};

class QoSShard;

class QoSPlayer : public enable_shared_from_this<QoSPlayer>
{
private:
	enum
	{
		QUEUE_MAX = 2,
	};
public:
	QoSPlayer(HostRef& owner, QoSShard* _shard, int32 tokenper, int32 burst) : _owner(owner), _ownerShard(_shard), _sendBucket(tokenper, burst) { cout << "QoSPlayer Added " << endl; }
	~QoSPlayer() { cout << "QoSPlayer Erased " << endl; }
	
	//void PushSend(SendBufferRef packet);
	void PushSend(shared_ptr<vector<SendBufferRef>> packet);
	
	void PopSend();
	
	
	void PushRecv(BYTE* buffer, int32 size);
	
	shared_ptr<SavedRecvPacket> PopRecv_RO();
	shared_ptr<SavedRecvPacket> PopRecv_URO();
	shared_ptr<SavedRecvPacket> PopRecv_RFCT();

	void OnOrderedRecv(int32 SeqNum, BYTE* buffer, int32 size);

	void ResetSendReady();
	void ResetRecvReady(); // Recv Logic Worker에서 일처리가 끝난 후 이제 이 Player를 안 쓸거라는 의미-> 다른 쓰레드에서 접근 가능
	
	

public:
	HostRef& GetOwner() { return _owner; }

	array<queue<shared_ptr<SavedSendPacket>>, QUEUE_MAX> _sendQueues;
	array<queue<shared_ptr<SavedRecvPacket>>, QUEUE_MAX> _recvQueues;
	shared_ptr<SavedSendPacket> _sendRPCTPacket_ptr;
	shared_ptr<SavedRecvPacket> _recvRPCTPacket_ptr;

	
private:
	//TokenBucket _recvBucket;
	HostRef _owner;
	QoSShard* _ownerShard;
	TokenBucket _sendBucket; // For Send 
	
	atomic<bool> _bRecvReady = false; // Shard에 자신이 넣어져있는지 확인하는 Flag -> 중복 push 방지 / RPCT는 제외기에 _sendRPCTPending으로 Flag처리
	atomic<bool> _bSendReady = false; 
	atomic<int32> _recvSumNum = 0;
	atomic<int32> _sendSumNum = 0;
	//atomic<bool> _sendRPCTPending = false;
	//atomic<bool> _recvRPCTPending = false;
	//atomic<bool> _bRecvWorkerUsing = false; //어떤한 RecvWorker가 사용중인지 알려주는 Flag
	/* For Fragment Handle Function And Values */
	Ordered_FG_Manager _fragmentManager;
	queue<shared_ptr<FragmentContext>> _orderedPacketQueue;
	USE_MANY_LOCKS(2);
};

class QoSShard
{


public:
	QoSShard();
	~QoSShard();

	void MakeQoSPlayer(HostRef object, int32 client_Id, int32 rate = 60, int32 burst = 10);
	void ErasePlayer(int32 client_Id);
	//void PushSend(int32 playerid, SendBufferRef packet);
	void PushSend(int32 client_Id, shared_ptr<vector<SendBufferRef>> packet);

	void PushRecv(int32 playerid, BYTE* buffer, int32 size);
	
	void OnOrderedRecv(int32 SeqNum, int32 playerid, BYTE* buffer, int32 size);

	void DoSendWork();
	//void DoRecvWork();
	void Stop();

	bool Empty_RecvReadyQueue();
	void PushRecvReadyQueue(const shared_ptr<QoSPlayer>& _player);
	shared_ptr<QoSPlayer> PopRecvReadyQueue();

	bool Empty_SendReadyQueue();
	void PushSendReadyQueue(const shared_ptr<QoSPlayer>& _player);
	shared_ptr<QoSPlayer> PopSendReadyQueue();
	//void PushSleepWorker(condition_variable& _cv);

	void AddReadyPlayerNum(int32 i);
	int32 GetRecvReadyPlayerNum() { return _recvWorkReadyPlayerNum.load(); }
	int32 GetSendReadyPlayerNum() { return _sendWorkReadyPlayerNum.load(); }
private:
	unordered_map<int32, shared_ptr<QoSPlayer>> _qosPlayers;
	queue<shared_ptr<QoSPlayer>> _recvReadyQueue; // queue for Players Stacked Packets in recvQueue;
	queue<shared_ptr<QoSPlayer>> _sendReadyQueue;
	atomic<int32> _recvWorkReadyPlayerNum = 0;
	atomic<int32> _sendWorkReadyPlayerNum = 0;
	//queue<condition_variable&> _sleepWorkers;
	atomic<bool> running = false;
	USE_MANY_LOCKS(3); // index(0) -> _qosPlayers를 위한것, index(1)-> RecvReadyQueue를 위한것,  index(2) ->RecvReadyQueue를 위한것,
};



class QoSCore
{
public:
	enum Channel : uint16
	{
		RO = 0, // Reliable Ordered Packet Channel
		URO = 1, // Unreliable Ordered Packet Channel //
		RPCT = 2, // Unreliable Ordered Packet Channel But For Packet used for Replication
		NO_CHANNEL = 3,
	};
	enum Priority: uint16
	{
		HIGH = 0,
		MEDIUM = 1,
		LOW = 2,
		PRIORTY_NULL = 3,
	};
	QoSCore();

	void OnRecv(int32 SeqNum, int32 client_Id, BYTE* buffer, int32 size);

	//void PushSend(int32 client_Id, SendBufferRef packet);
	void PushSend(int32 client_Id, shared_ptr<vector<SendBufferRef>> packet);

	void PushRecv(int32 client_Id, BYTE* buffer, int32 size);

	void OnOrderedRecv(int32 SeqNum, int32 client_Id, BYTE* buffer, int32 size);

	void StopShards();
	void ErasePlayer(int32 id);
	QoSShard* GetShard(int32 playerid);
	QoSShard* GetShard_index(int32 index);
	QoSShard* GetBusyShard_RCV(); // RecvWorker에서 자신이 담당하는 Shard의 recvReadyQueue가 비어있을때 도움이 필요한 다른 Shard를 찾는 함수
	QoSShard* GetBusyShard_SEND(); // SendWorker에서 자신이 담당하는 Shard의 sendReadyQueue가 비어있을때 도움이 필요한 다른 Shard를 찾는 함수


	std::mutex& GetRecvMutex() { return _recvMutex; }
	std::condition_variable& GetRecvCV() { return _recvCv; }
	std::mutex& GetSendMutex() { return _sendMutex; }
	std::condition_variable& GetSendCV() { return _sendCv; }
private:
	
	array<unique_ptr<QoSShard>, QOS_SHARD_COUNT> _qosShards;
	//void PopSend();
	int32 GetShardIndex(int32 playerid) { return playerid % QOS_SHARD_COUNT; }
	
	std::mutex _recvMutex;
	std::condition_variable _recvCv;
	std::mutex _sendMutex;
	std::condition_variable _sendCv;
};

