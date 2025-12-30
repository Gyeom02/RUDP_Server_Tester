#pragma once
//Class For Quality of Service
#include "Nids.h"
#include <map>
#include "Object.h"
#include "Lock.h"


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
struct SavedPacket
{
	SendBufferRef sendBuffer;
	ObjectRef object;
	SavedPacket(ObjectRef s, SendBufferRef buffer) : object(s), sendBuffer(buffer) {}
};

class QoSPlayer
{
public:
	QoSPlayer(int32 tokenper, int32 burst) : bucket(tokenper, burst) { cout << "QoSPlayer Added " << endl; }
	~QoSPlayer() { cout << "QoSPlayer Erased " << endl; }

	void PopSend();
	void Push(ObjectRef object, SendBufferRef packet);
	array<queue<shared_ptr<SavedPacket>>, 3> _queues;

private:
	TokenBucket bucket;
	USE_LOCK;
};

class QoSShard
{
public:
	QoSShard();
	~QoSShard();

	void MakeQoSPlayer(int32 playerId, int32 rate = 60, int32 burst = 10);
	void ErasePlayer(int32 playerId);
	void Push(int32 playerid, ObjectRef object, SendBufferRef packet);
	void DoWork();
	void Stop() { running = false; }
private:
	unordered_map<int32, shared_ptr<QoSPlayer>> _qosPlayers;
	bool running = false;
	USE_LOCK;
};
class QoSCore
{
public:
	enum Priority: uint16
	{
		HIGH = 0,
		MEDIUM = 1,
		LOW = 2,
		FPC = 3, //Fastest Possibly Can -> Usual Used For Replication Packets
		MAX = 3,
	};
	QoSCore();
	void Push(int32 playerId, ObjectRef object, SendBufferRef packet);
	
	void StopShards();
	void ErasePlayer(int32 id);
	QoSShard* GetShard(int32 playerid);
private:
	
	array<unique_ptr<QoSShard>, QOS_SHARD_COUNT> _qosShards;
	void PopSend();
	int32 GetShardIndex(int32 playerid) { return playerid % QOS_SHARD_COUNT; }
	

};

