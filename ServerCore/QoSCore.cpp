#include "pch.h"
#include "QoSCore.h"
#include "ThreadManager.h"


QoSShard* QoSCore::GetShard(int32 playerid)
{
	return _qosShards[GetShardIndex(playerid)].get();
}

QoSShard* QoSCore::GetShard_index(int32 index)
{
	if (index >= QOS_SHARD_COUNT)
#ifdef _DEBUG
		_ASSERT(false);
#else
		return nullptr;
#endif
	return _qosShards[index].get();
}

QoSShard* QoSCore::GetBusyShard_RCV()
{
	QoSShard* busyShard = nullptr;
	int busy_index = -1;
	int max = 0;
	for (int i = 0; i < QOS_SHARD_COUNT; i++)
	{
		int num = GetShard_index(i)->GetRecvReadyPlayerNum();
		if (max < num)
		{
			max = num;
			busy_index = i;
		}
	}
	if (busy_index == -1)
		return nullptr;
	return GetShard_index(busy_index);
}

QoSShard* QoSCore::GetBusyShard_SEND()
{
	QoSShard* busyShard = nullptr;
	int busy_index = -1;
	int max = 0;
	for (int i = 0; i < QOS_SHARD_COUNT; i++)
	{
		int num = GetShard_index(i)->GetSendReadyPlayerNum();
		if (max < num)
		{
			max = num;
			busy_index = i;
		}
	}
	if (busy_index == -1)
		return nullptr;
	return GetShard_index(busy_index);
}

void QoSPlayer::PushSend(SendBufferRef packet)
{
	bool expected = false;
	
	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet->Buffer());

	uint16 channel = header->channel;
	if (channel == QoSCore::RO)
	{
		SEND_WRITE_LOCK;
		_sendQueues[QoSCore::RO].push(MakeShared<SavedSendPacket>(packet));
		_sendSumNum.fetch_add(1);
	}
	else if (channel == QoSCore::URO)
	{
		SEND_WRITE_LOCK;
		_sendQueues[QoSCore::URO].push(MakeShared<SavedSendPacket>(packet));
		_sendSumNum.fetch_add(1);
	}
	else if (channel == QoSCore::RPCT)
	{
		SEND_WRITE_LOCK;
		if(_sendRPCTPacket_ptr == nullptr)
			_sendSumNum.fetch_add(1);
		_sendRPCTPacket_ptr = MakeShared<SavedSendPacket>(packet);
		
	}
	else
		return;

	

	

	if (_bSendReady.compare_exchange_strong(expected, true))
	{
	//	cout << "PushSendReadyQueue 1" << endl;
		_ownerShard->PushSendReadyQueue(shared_from_this());
	}
}



void QoSPlayer::PopSend()
{
	shared_ptr<SavedSendPacket> savePacket = nullptr; //기아 현상 해결해야함
	{
		int coin = 5;
		DeliveryManagerRef dm = _owner->GetDeliveryManager();
		while (coin--)
		{
			
			
			{
				SEND_WRITE_LOCK;
				if (_sendQueues[QoSCore::RO].empty() || !_sendBucket.Consume())
					break;

				savePacket = _sendQueues[QoSCore::RO].front();

				if (!dm->IsSpaceExistToSend(savePacket->sendBuffer->WriteSize())) // 상대방의 rwind가 보내려는 패킷의 사이즈보다 작음(보낼 수 없음 Flow-Control)
				{
#ifdef _DEBUG
					HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
					CONSOLE_SCREEN_BUFFER_INFO info;
					//GetConsoleScreenBufferInfo(h, &info);
					COORD pos = { (SHORT)0, (SHORT)11};
					SetConsoleCursorPosition(h, pos);

					cout << "Out Of RWind : " << dm->GetReceiverRWind() << " < " << savePacket->sendBuffer->WriteSize() << endl;

#endif
					break;
				}
				_sendQueues[QoSCore::RO].pop();
				
				
			}
				
			//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());
			_sendSumNum.fetch_sub(1);
			_owner->PriortySend(savePacket->sendBuffer);
			
		}
		 coin = 3;
		while (coin--)
		{
			
			{

				SEND_WRITE_LOCK;
				if (_sendQueues[QoSCore::URO].empty() || !_sendBucket.Consume())
					break;
				
				savePacket = _sendQueues[QoSCore::URO].front();
				_sendQueues[QoSCore::URO].pop();
				
			
			}
				
			//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());
			_sendSumNum.fetch_sub(1);
			_owner->PriortySend(savePacket->sendBuffer);
			
		}
		
		
		{
			SEND_WRITE_LOCK;
			if (_sendRPCTPacket_ptr == nullptr || !_sendBucket.Consume())
			{
				//_sendRPCTPending.store(false);
				ResetSendReady();
				return;
			}//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());
			
			savePacket = _sendRPCTPacket_ptr;
			_sendRPCTPacket_ptr = nullptr;
			
			_sendSumNum.fetch_sub(1);

			
		}
		_owner->PriortySend(savePacket->sendBuffer);
		//_sendSumNum.fetch_add(-1);
		
				
		
		ResetSendReady();
	}
	

}


void QoSPlayer::PushRecv(BYTE* buffer, int32 size)
{
	bool expected = false;
	
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	uint16 channel = header->channel;

	
	if (channel == QoSCore::RO)
	{
		RECV_WRITE_LOCK;
		_recvQueues[QoSCore::RO].push(MakeShared<SavedRecvPacket>(buffer, size));
		_recvSumNum.fetch_add(1);
	}
	else if (channel == QoSCore::URO)
	{
		RECV_WRITE_LOCK;
		_recvQueues[QoSCore::URO].push(MakeShared<SavedRecvPacket>(buffer, size));
		_recvSumNum.fetch_add(1);
	}
	else if (channel == QoSCore::RPCT)
	{
		RECV_WRITE_LOCK;
		if(_recvRPCTPacket_ptr == nullptr)
			_recvSumNum.fetch_add(1);
		_recvRPCTPacket_ptr = MakeShared<SavedRecvPacket>(buffer, size);
		
	}
	else
		return;

	

	

	if (_bRecvReady.compare_exchange_strong(expected, true))
	{
		//cout << "PushRecvReadyQueue 1" << endl;
		_ownerShard->PushRecvReadyQueue(shared_from_this());
	}
}


shared_ptr<SavedRecvPacket> QoSPlayer::PopRecv_RO()
{
	shared_ptr<SavedRecvPacket> savePacket;
	{
		RECV_READ_LOCK;
		if (_recvQueues[QoSCore::RO].empty())
			return nullptr;
	}
	{
		RECV_WRITE_LOCK;
		
		savePacket = _recvQueues[QoSCore::RO].front();
		_recvQueues[QoSCore::RO].pop();
		
#ifdef _DEBUG
		_ASSERT(savePacket != nullptr);
#else
#endif
	}

	/*buffer = savePacket->_buffer;
	size = savePacket->_size;*/
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());	
	_recvSumNum.fetch_sub(1);
	return savePacket;

}

shared_ptr<SavedRecvPacket> QoSPlayer::PopRecv_URO()
{
	shared_ptr<SavedRecvPacket> savePacket;
	{
		RECV_READ_LOCK;
		if (_recvQueues[QoSCore::URO].empty())
			return nullptr;
	}
	{
		RECV_WRITE_LOCK;

		savePacket = _recvQueues[QoSCore::URO].front();
		_recvQueues[QoSCore::URO].pop();

#ifdef _DEBUG
		_ASSERT(savePacket != nullptr);
#else
#endif
	}

	/*buffer = savePacket->_buffer;
	size = savePacket->_size;*/
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());	
	_recvSumNum.fetch_sub(1);
	return savePacket;
}

shared_ptr<SavedRecvPacket> QoSPlayer::PopRecv_RFCT()
{

	RECV_WRITE_LOCK;
		
	if (!_recvRPCTPacket_ptr)
	{
		//_recvRPCTPending.store(false);
		return nullptr;

	}	
	shared_ptr<SavedRecvPacket> copy = _recvRPCTPacket_ptr;
	/*buffer = _recvRPCTPacket_ptr->_buffer;
	size = _recvRPCTPacket_ptr->_size;*/
	_recvSumNum.fetch_sub(1);

	_recvRPCTPacket_ptr = nullptr;

	//_recvRPCTPending.store(false);
	//_recvSumNum.fetch_add(-1);
	return copy;
	

	//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());	

	
}

void QoSPlayer::OnOrderedRecv(int32 SeqNum, BYTE* buffer, int32 size)
{
	
	//int PacketNum = 0;
	DeliveryManagerRef dm = _owner->GetDeliveryManager();
	if (_fragmentManager.OnOrderedRecv(SeqNum, buffer, size, _orderedPacketQueue))
	{
		while (!_orderedPacketQueue.empty())
		{
			
			vector<BYTE>& v = _orderedPacketQueue.front();
			PushRecv(v.data(), v.size());
			dm->MakeSpaceRWind(v.size()); // RecvBuffer에서 Logic 처리로 옮겨졌기때문에 RWind의 크기를 해당 패킷 사이즈 만큼 다시 넓혀줘야 받을 수 있음
			_orderedPacketQueue.pop();
		}
	}

}


void QoSPlayer::ResetSendReady()
{
	//_bSendReady.store(false);

	// 2) 그 사이에 새 작업이 들어왔거나, 아직 작업이 남아있으면 다시 올린다
	int expectNum = 0;
	if (_sendSumNum.compare_exchange_strong(expectNum, 0))
	{
		_bSendReady.store(false);
	}
	else {
		_ownerShard->PushSendReadyQueue(shared_from_this()); //PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	}
}

void QoSPlayer::ResetRecvReady()
{
	int expectNum = 0;
	if (_recvSumNum.compare_exchange_strong(expectNum, 0))
	{
		_bRecvReady.store(false);
	}
	else {
		//cout << "expectNum : " << expectNum << endl;
		_ownerShard->PushRecvReadyQueue(shared_from_this()); //PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	}
}

void QoSShard::MakeQoSPlayer(ObjectRef object, int32 playerId, int32 rate, int32 burst)
{
	WRITE_LOCK;

	//TODO 만약 같은 id를 사용하기 원하는 Player가 나왔을때 그냥 무시할 것인지 아님 emplace + check을 고려할 것인지 생각
	_qosPlayers[playerId] = MakeShared<QoSPlayer>(object, this, rate, burst);
}

void QoSShard::ErasePlayer(int32 playerId)
{
	WRITE_LOCK;
	/*shared_ptr<QoSPlayer> player;
	auto iterator = _qosPlayers.find(playerId);
	if (iterator != _qosPlayers.end())
		player = iterator->second;*/
	_qosPlayers.erase(playerId);
	

}

void QoSShard::PushSend(int32 playerid, SendBufferRef packet)
{
	shared_ptr<QoSPlayer> qosplaayer;
	
	{
		READ_LOCK;
		auto iter  = _qosPlayers.find(playerid);


		if (iter == _qosPlayers.end())
			return;

		
		qosplaayer = iter->second;
	}

	
	qosplaayer->PushSend(packet);
}

void QoSShard::PushRecv(int32 playerid, BYTE* buffer, int32 size)
{

	shared_ptr<QoSPlayer> qosplaayer;

	{
		READ_LOCK;
		auto iter = _qosPlayers.find(playerid);


		if (iter == _qosPlayers.end())
			return;

		qosplaayer = iter->second;
	}

	
	qosplaayer->PushRecv(buffer, size);


}

void QoSShard::OnOrderedRecv(int32 SeqNum, int32 playerid, BYTE* buffer, int32 size)
{
	shared_ptr<QoSPlayer> qosplaayer;

	{
		READ_LOCK;
		auto iter = _qosPlayers.find(playerid);


		if (iter == _qosPlayers.end())
			return;

		qosplaayer = iter->second;
	}
	qosplaayer->OnOrderedRecv(SeqNum, buffer, size);
}

QoSCore::QoSCore()
{
	for (int i = 0; i < QOS_SHARD_COUNT; i++)
	{
		_qosShards[i] = make_unique<QoSShard>();
	}
}

void QoSCore::OnRecv(int32 SeqNum, int32 playerId, BYTE* buffer, int32 size)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	if (header->channel == QoSCore::Channel::RO) // Reliable Ordered
	{
		
		OnOrderedRecv(SeqNum, playerId, buffer, size);
	
	}
	else // UnReliable Ordered or RFCT
	{
		PushRecv(playerId, buffer, size);
	}
}

void QoSCore::PushSend(int32 playerId, SendBufferRef packet)
{
	QoSShard* shard = GetShard(playerId);
	shard->PushSend(playerId,  packet);
	
}

void QoSCore::PushRecv(int32 playerId, BYTE* buffer, int32 size)
{
	QoSShard* shard = GetShard(playerId);
	shard->PushRecv(playerId, buffer, size);

}

void QoSCore::OnOrderedRecv(int32 SeqNum, int32 playerId, BYTE* buffer, int32 size)
{
	QoSShard* shard = GetShard(playerId);
	shard->OnOrderedRecv(SeqNum, playerId, buffer, size);
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

void QoSShard::DoSendWork()
{
	//std::vector<std::shared_ptr<QoSPlayer>> snapshot;
	//snapshot.reserve(1024);
	//while (running)
	//{
	//	//cout << " QoSCore::DoWork" << endl;
	//	snapshot.clear();
	//	{
	//		READ_LOCK;
	//		
	//		snapshot.reserve(_sendReadyQueue.size());
	//		for (auto& qosplayer : _sendReadyQueue)
	//		{
	//			snapshot.push_back(qosplayer);
	//		}
	//	}
	//	for(auto& player : snapshot)
	//	{
	//		if (player)
	//			player->PopSend();
	//	}
	//}
	while (running)
	{
		shared_ptr<QoSPlayer> _player = PopSendReadyQueue();
		if (!_player) {
			//TODO Wait or Do Something
			QoSShard* _busyShard = GQoS->GetBusyShard_SEND();

			if (!_busyShard)
			{
				continue;
				//this_thread::sleep_for(2ms);
			}
			_player = _busyShard->PopSendReadyQueue();
			if (!_player)
			{
				//SleepTillGetSignal();
				continue;

			}
		}
		_player->PopSend();
		
	}
}

void QoSShard::PushRecvReadyQueue(const shared_ptr<QoSPlayer>& _player)
{
	WRITE_LOCK_IDX(1);
	_recvReadyQueue.push(_player);
	_recvWorkReadyPlayerNum.fetch_add(1);
	//cout << "PushRecvReadyQueue" << endl;
	/*if (!_sleepWorkers.empty()) 
	{
		_sleepWorkers.front().notify_one();
		_sleepWorkers.pop();
	}*/
}

shared_ptr<QoSPlayer> QoSShard::PopRecvReadyQueue()
{
	shared_ptr<QoSPlayer> returnPtr = nullptr;
	{
		WRITE_LOCK_IDX(1);
		if (_recvReadyQueue.empty())
			return nullptr;
		returnPtr = _recvReadyQueue.front();
		_recvReadyQueue.pop();
	}
#ifdef _DEBUG
	_ASSERT(returnPtr);
#else
#endif
	//cout << "PopRecvReadyQueue" << endl;

	_recvWorkReadyPlayerNum.fetch_add(-1);
	return returnPtr;
}

void QoSShard::PushSendReadyQueue(const shared_ptr<QoSPlayer>& _player)
{
	WRITE_LOCK_IDX(2);
	_sendReadyQueue.push(_player);
	_sendWorkReadyPlayerNum.fetch_add(1);
	//_recvWorkReadyPlayerNum.fetch_add(1);
	//cout << "PushSendReadyQueue" << endl;
}

shared_ptr<QoSPlayer> QoSShard::PopSendReadyQueue()
{
	shared_ptr<QoSPlayer> returnPtr = nullptr;
	{
		WRITE_LOCK_IDX(2);
		if (_sendReadyQueue.empty())
			return nullptr;
		returnPtr = _sendReadyQueue.front();
		_sendReadyQueue.pop();
	}
#ifdef _DEBUG
	_ASSERT(returnPtr);
#else
#endif
//	cout << "PopSendReadyQueue" << endl;
	_sendWorkReadyPlayerNum.fetch_add(-1);
	return returnPtr;
}

//void QoSShard::PushSleepWorker(condition_variable& _cv)
//{
//	WRITE_LOCK_IDX(1);
//
//	_sleepWorkers.push(_cv);
//}

void QoSShard::AddReadyPlayerNum(int32 i)
{
	_recvWorkReadyPlayerNum.fetch_add(i);
}

//void QoSShard::DoRecvWork()
//{
//	while (running)
//	{
//		//cout << " QoSCore::DoWork" << endl;
//		for (auto [id, qosplayer] : _qosPlayers)
//		{
//			if (qosplayer)
//				qosplayer->PopRecv();
//		}
//	}
//}


QoSShard::QoSShard()
	: running(true)
{
	GThreadManager->Launch([this]() {
		DoSendWork();
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

SavedRecvPacket::SavedRecvPacket(BYTE* buffer, int32 size)
{
	_buffer = new BYTE[size];
	::memcpy(_buffer, buffer, size);
	_size = size;
}

SavedRecvPacket::~SavedRecvPacket()
{
	delete[] _buffer;
	_buffer = nullptr;
}
