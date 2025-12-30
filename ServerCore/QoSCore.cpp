#include "pch.h"
#include "QoSCore.h"
#include "ThreadManager.h"


void QoSPlayer::Push(ObjectRef object, SendBufferRef packet)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet->Buffer());

	uint16 priority = header->priority;
	if (priority == QoSCore::HIGH)
	{
		WRITE_LOCK;
		_queues[QoSCore::HIGH].push(MakeShared<SavedPacket>(object, packet));
	}
	else if (priority == QoSCore::MEDIUM)
	{
		WRITE_LOCK;
		_queues[QoSCore::MEDIUM].push(MakeShared<SavedPacket>(object, packet));
	}
	else if (priority == QoSCore::LOW)
	{
		WRITE_LOCK;
		_queues[QoSCore::LOW].push(MakeShared<SavedPacket>(object, packet));
	}
	else
		return;
}

QoSShard* QoSCore::GetShard(int32 playerid)
{
	return _qosShards[GetShardIndex(playerid)].get();
}

void QoSPlayer::PopSend()
{
	shared_ptr<SavedPacket> savePacket = nullptr; //기아 현상 해결해야함
	{
		int coin = 5;
		while (!_queues[QoSCore::HIGH].empty() && coin--)
		{
			if (bucket.Consume())
			{
				savePacket = _queues[QoSCore::HIGH].front();
				_queues[QoSCore::HIGH].pop();
				if (savePacket == nullptr)
					return;
				//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());

				savePacket->object->PriortySend(savePacket->sendBuffer);
			}
		}
		 coin = 3;
		while (!_queues[QoSCore::MEDIUM].empty() && coin--)
		{
			if (bucket.Consume())
			{
				savePacket = _queues[QoSCore::MEDIUM].front();
				_queues[QoSCore::MEDIUM].pop();
				if (savePacket == nullptr)
					return;
				//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());

				savePacket->object->PriortySend(savePacket->sendBuffer);
			}
		}
		 coin = 2;
		while (!_queues[QoSCore::LOW].empty() && coin--)
		{
			if (bucket.Consume())
			{
				savePacket = _queues[QoSCore::LOW].front();
				_queues[QoSCore::LOW].pop();
				if (savePacket == nullptr)
					return;
				//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());

				savePacket->object->PriortySend(savePacket->sendBuffer);
			}
		}
	}
	

}

void QoSShard::MakeQoSPlayer(int32 playerId, int32 rate, int32 burst)
{
	_qosPlayers[playerId] = MakeShared<QoSPlayer>(rate, burst);
}

void QoSShard::ErasePlayer(int32 playerId)
{
	WRITE_LOCK;
	_qosPlayers.erase(playerId);
}

void QoSShard::Push(int32 playerid, ObjectRef object, SendBufferRef packet)
{
	WRITE_LOCK;
	
	shared_ptr<QoSPlayer>& qosplaayer = _qosPlayers[playerid];
	qosplaayer->Push(object, packet);
}



QoSCore::QoSCore()
{
	for (int i = 0; i < QOS_SHARD_COUNT; i++)
	{
		_qosShards[i] = make_unique<QoSShard>();
	}
}

void QoSCore::Push(int32 playerId, ObjectRef object, SendBufferRef packet)
{
	QoSShard* shard = GetShard(playerId);
	shard->Push(playerId, object, packet);
	
}

void QoSCore::StopShards()
{
	for (int i = 0; i < QOS_SHARD_COUNT; i++)
		GetShard(i)->Stop();
}

void QoSCore::ErasePlayer(int32 id)
{
	GetShard(id)->ErasePlayer(id);
}

void QoSShard::DoWork()
{
	while (running)
	{
		//cout << " QoSCore::DoWork" << endl;
		for(auto [id, qosplayer] : _qosPlayers)
			{
			if(qosplayer)
				qosplayer->PopSend();
			}
	}
}


QoSShard::QoSShard()
	: running(true)
{
	GThreadManager->Launch([this]() {
		DoWork();
		});
}

QoSShard::~QoSShard()
{
	Stop();
}



void TokenBucket::Refil()
{
	TimePoint now = chrono::steady_clock::now();
	int32 elapseMs = chrono::duration_cast<chrono::milliseconds>(now - lastRefil).count();
	int32 refill = (elapseMs * refilRate) / 1000;
	if (refill > 0)
	{
		tokens = maxTokens < refill + tokens ? maxTokens : refill + tokens;
		lastRefil = now;
	}
}

bool TokenBucket::Consume()
{
	Refil();
	if (tokens > 0)
	{
		tokens--;
		return true;
	}
	return false;
}

