#include "pch.h"
#include "QoSCore.h"
#include "ThreadManager.h"
#include "RTTManager.h"

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
//
//void QoSPlayer::PushSend(SendBufferRef packet)
//{
//	bool expected = false;
//	
//	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet->Buffer());
//
//	uint16 channel = header->channel;
//	if (channel == QoSCore::RO)
//	{
//		SEND_WRITE_LOCK;
//		_sendQueues[QoSCore::RO].push(MakeShared<SavedSendPacket>(packet));
//		_sendSumNum.fetch_add(1);
//	}
//	else if (channel == QoSCore::URO)
//	{
//		SEND_WRITE_LOCK;
//		_sendQueues[QoSCore::URO].push(MakeShared<SavedSendPacket>(packet));
//		_sendSumNum.fetch_add(1);
//	}
//	else if (channel == QoSCore::RPCT)
//	{
//		SEND_WRITE_LOCK;
//		if(_sendRPCTPacket_ptr == nullptr)
//			_sendSumNum.fetch_add(1);
//		_sendRPCTPacket_ptr = MakeShared<SavedSendPacket>(packet);
//		
//	}
//	else
//		return;
//
//	
//
//	
//
//	if (_bSendReady.compare_exchange_strong(expected, true))
//	{
//	//	cout << "PushSendReadyQueue 1" << endl;
//		_ownerShard->PushSendReadyQueue(shared_from_this());
//	}
//}

QoSPlayer::QoSPlayer(HostRef& owner, QoSShard* _shard, int32 tokenper, int32 burst)
	: _owner(owner), _ownerShard(_shard), _sendBucket(tokenper, burst) 
{
	cout << "QoSPlayer Added " << endl;

	InitSendQueuesOutStand();
}

void QoSPlayer::PushSend(shared_ptr<vector<SendBufferRef>> packet)
{
	bool expected = false;

	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet->front()->Buffer());

	uint16 channel = header->channel;
	if (channel == QoS::RO)
	{
		SEND_WRITE_LOCK;
		_sendQueues[QoS::RO].Push((MakeShared<SavedSendPacket>(packet)), header->priority);
		_sendSumNum.fetch_add(1);
	}
	else if (channel == QoS::URO)
	{
		SEND_WRITE_LOCK;
		_sendQueues[QoS::URO].Push((MakeShared<SavedSendPacket>(packet)), header->priority);
		_sendSumNum.fetch_add(1);
	}
	else if (channel == QoS::RPCT)
	{
		SEND_WRITE_LOCK;
		if (_sendRPCTPacket_ptr == nullptr)
			_sendSumNum.fetch_add(1);
		_sendRPCTPacket_ptr = MakeShared<SavedSendPacket>(packet);

	}
	else
		return;





	if (_bSendReady.compare_exchange_strong(expected, true))
	{
		//	cout << "PushSendReadyQueue 1" << endl;
		_ownerShard->PushSendReadyQueue(shared_from_this());
		GQoS->GetSendCV().notify_one();
	}
}

void QoSPlayer::PopSend()
{
	shared_ptr<SavedSendPacket> savePacket = nullptr; //기아 현상 해결해야함
	uint16 selected_priorty = -1;
	{
		int coin = 5;
		DeliveryManagerRef dm = _owner->GetDeliveryManager();
		while (coin--)
		{
			
			
			{
				SEND_WRITE_LOCK;
				auto& ROQueue = _sendQueues[QoS::RO]._queue;
				
				
				/*if (ROQueue.empty() || )
					break;*/
					//auto& ResendQueue = _sendQueues[QoSCore::RO]._queue[QoSCore::Priority::RESEND];
				if (!ROQueue[QoS::Priority::RESEND].empty())
				{
					selected_priorty = QoS::Priority::RESEND;
				}
				else if (!ROQueue[QoS::Priority::HIGH].empty())
				{
					selected_priorty = QoS::Priority::HIGH;
				}
				else if (!ROQueue[QoS::Priority::MEDIUM].empty())
				{

					selected_priorty = QoS::Priority::MEDIUM;
				}
				else if (!ROQueue[QoS::Priority::LOW].empty())
				{
					selected_priorty = QoS::Priority::LOW;
				}
				else break;

				if (!_sendBucket.Consume())
					break;

				savePacket = ROQueue[selected_priorty].front();
				
				PacketHeader* header = reinterpret_cast<PacketHeader*>((*savePacket->sendBuffers)[0]->Buffer());

				_sendQueues[QoS::RO].UpdateOutStandBudget();

				if (!_sendQueues[QoS::RO].CheckHas_OutStandBudget(savePacket->AllBuffersSize))
					break;

				_owner->GetRTTManager().UpdateCongestBudget();
				if (!_owner->GetRTTManager().ValidBudget(savePacket->AllBuffersSize))
				{

					break;
				}
				if (header->retransnum <= 0 )//재전송 패킷이면 RWind 로직 패스(이미 처음 보낼때 패킷 사이즈만큼 RWind 처리했기때문)
				{
					if (header->priority == QoS::Priority::RESEND)
					{
						
						ROQueue[selected_priorty].pop();
						_sendSumNum.fetch_sub(1);
						break;
					}
				
					
					if (!dm->IsSpaceExistToSend(savePacket->AllBuffersSize)) // 상대방의 rwind가 보내려는 패킷의 사이즈보다 작음(보낼 수 없음 Flow-Control)
					{			
						//InFlightPacketPtr inflight = dm->FindOldestInFlightPacket();
						//if (!inflight)
						//{
						////	cout << "oldest inflight not exisit | ID : " << _owner->client_Id  << endl;
						//	break;
						//}
						//double ResendDelay = _owner->GetRTTManager().GetResendDelay(reinterpret_cast<PacketHeader*>(inflight->GetTransmissionData()->Buffer())->retransnum);
						//if (double(UTime::GetNow() - inflight->_time_recent_send.load()) > ResendDelay)
						//{
						//	//reinterpret_cast<PacketHeader*>(inflight->GetTransmissionData()->Buffer())->sn = inflight->GetSequenceNumber().GetSN(); // 임시
						////	cout << " HandlePacketDeliveryFailure ID : " << _owner->client_Id << " | InFlight SN : " << inflight->GetSequenceNumber().GetSN() << " | Resend Packet SN : " << reinterpret_cast<PacketHeader*>(inflight->GetTransmissionData()->Buffer())->sn << endl;
						//	
						//	dm->HandlePacketDeliveryFailure(inflight);
						//}
						break;
					}
					
				}
				_sendQueues[QoS::RO].Use_OutStandBudget(savePacket->AllBuffersSize);
				_owner->GetRTTManager().UseBudget(savePacket->AllBuffersSize);


				//cout << "savePacket->sendBuffers->size() : " << savePacket->sendBuffers->size() << endl;
				//cout << "Send Client ID : " << _owner->client_Id << " |  Packet SN: " << header->sn << endl;

				
				//savePacket->SendStartIndex++;
				/*if (!savePacket->bSend)
					savePacket->bSend = true;
				else
					cout << "A" << endl;*/
				ROQueue[selected_priorty].pop();
				_sendSumNum.fetch_sub(1);
				//savePacket->bSend = true;
				
			}
			_owner->PriortySend(savePacket->sendBuffers);
			//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());
			
			
		}
		 coin = 3;
		while (coin--)
		{
			auto& UROQueue = _sendQueues[QoS::URO]._queue;
			{
				

				SEND_WRITE_LOCK;
				/*if (ROQueue.empty() || )
					break;*/
					//auto& ResendQueue = _sendQueues[QoSCore::RO]._queue[QoSCore::Priority::RESEND];
				if (!UROQueue[QoS::Priority::RESEND].empty())
				{
					selected_priorty = QoS::Priority::RESEND;
				}
				else if (!UROQueue[QoS::Priority::HIGH].empty())
				{
					selected_priorty = QoS::Priority::HIGH;
				}
				else if (!UROQueue[QoS::Priority::MEDIUM].empty())
				{
					selected_priorty = QoS::Priority::MEDIUM;
				}
				else if (!UROQueue[QoS::Priority::LOW].empty())
				{
					selected_priorty = QoS::Priority::LOW;
				}
				else break;

				if (!_sendBucket.Consume())
					break;
				////////
				
				
				savePacket = UROQueue[selected_priorty].front();
				//int32 startindex = savePacket->SendStartIndex;
				//bool bAllSend = true;
				_sendQueues[QoS::URO].UpdateOutStandBudget();

				if (!_sendQueues[QoS::URO].CheckHas_OutStandBudget(savePacket->AllBuffersSize))
					break;


				_owner->GetRTTManager().UpdateCongestBudget();
				if (!_owner->GetRTTManager().ValidBudget(savePacket->AllBuffersSize))
				{
					
					break;
				}
				//if (!dm->IsSpaceExistToSend(savePacket->AllBuffersSize)) // 상대방의 rwind가 보내려는 패킷의 사이즈보다 작음(보낼 수 없음 Flow-Control)
				//{
				//	//	bAllSend = false;
				//	break;
				//}
				_sendQueues[QoS::URO].Use_OutStandBudget(savePacket->AllBuffersSize);
				_owner->GetRTTManager().UseBudget(savePacket->AllBuffersSize);

				
				//savePacket->SendStartIndex++;

				UROQueue[selected_priorty].pop();
				_sendSumNum.fetch_sub(1);
				
			
			}
			_owner->PriortySend(savePacket->sendBuffers);
			//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());
			
			
		}
		
		bool cansend = false;
		{
			SEND_WRITE_LOCK;
			
			if (_sendRPCTPacket_ptr == nullptr || !_sendBucket.Consume())
			{
				//_sendRPCTPending.store(false);
				ResetSendReady();
				return;
			}//PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());
			
			savePacket = _sendRPCTPacket_ptr;

			_owner->GetRTTManager().UpdateCongestBudget();
			
			if (_owner->GetRTTManager().ValidBudget(savePacket->AllBuffersSize) && dm->IsSpaceExistToSend(savePacket->AllBuffersSize)) // 상대방의 rwind가 보내려는 패킷의 사이즈보다 작음(보낼 수 없음 Flow-Control)
			{
				_owner->GetRTTManager().UseBudget(savePacket->AllBuffersSize);
				_sendRPCTPacket_ptr = nullptr;

				_sendSumNum.fetch_sub(1);
				cansend = true;
			}
			

			
		}
		if(cansend)
			_owner->PriortySend(savePacket->sendBuffers);
		//_sendSumNum.fetch_add(-1);
		
				
		
		ResetSendReady();
	}
	

}


void QoSPlayer::PushRecv(BYTE* buffer, int32 size)
{
	bool expected = false;
	
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	uint16 channel = header->channel;

	
	if (channel == QoS::RO)
	{
		RECV_WRITE_LOCK;
		_recvQueues[QoS::RO].Push(MakeShared<SavedRecvPacket>(buffer, size), header->priority);
		_recvSumNum.fetch_add(1);
	}
	else if (channel == QoS::URO)
	{
		RECV_WRITE_LOCK;
		_recvQueues[QoS::URO].Push(MakeShared<SavedRecvPacket>(buffer, size), header->priority);
		_recvSumNum.fetch_add(1);
	}
	else if (channel == QoS::RPCT)
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
		GQoS->GetRecvCV().notify_one();
	}
}


shared_ptr<SavedRecvPacket> QoSPlayer::PopRecv_RO()
{
	shared_ptr<SavedRecvPacket> savePacket;
	uint16 selected_priorty = -1;
	{
		RECV_WRITE_LOCK;

		auto& ROQueue = _recvQueues[QoS::RO]._queue;

		if (!ROQueue[QoS::Priority::RESEND].empty())
		{
			selected_priorty = QoS::Priority::RESEND;
		}
		else if (!ROQueue[QoS::Priority::HIGH].empty())
		{
			selected_priorty = QoS::Priority::HIGH;
		}
		else if (!ROQueue[QoS::Priority::MEDIUM].empty())
		{
			selected_priorty = QoS::Priority::MEDIUM;
		}
		else if (!ROQueue[QoS::Priority::LOW].empty())
		{
			selected_priorty = QoS::Priority::LOW;
		}
		else return nullptr;

		////
		
		savePacket = ROQueue[selected_priorty].front();
		ROQueue[selected_priorty].pop();
		
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
	uint16 selected_priorty = -1;
	{
		RECV_WRITE_LOCK;

		auto& UROQueue = _recvQueues[QoS::URO]._queue;

		if (!UROQueue[QoS::Priority::RESEND].empty())
		{
			selected_priorty = QoS::Priority::RESEND;
		}
		else if (!UROQueue[QoS::Priority::HIGH].empty())
		{
			selected_priorty = QoS::Priority::HIGH;
		}
		else if (!UROQueue[QoS::Priority::MEDIUM].empty())
		{
			selected_priorty = QoS::Priority::MEDIUM;
		}
		else if (!UROQueue[QoS::Priority::LOW].empty())
		{
			selected_priorty = QoS::Priority::LOW;
		}
		else return nullptr;

		////

		savePacket = UROQueue[selected_priorty].front();
		UROQueue[selected_priorty].pop();

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
		int32 allSize = 0;
		while (!_orderedPacketQueue.empty())
		{
			
			shared_ptr<FragmentContext> orderedPacket = _orderedPacketQueue.front();
			int32 realPacketSize = 0; // fragment 패킷이라면 fragment Header 사이즈 * fragment 개수 가 추가적으로 더해진 진짜 버퍼 사이즈 값

			vector<BYTE>& storedbuffer = orderedPacket->_storedBuffer;
			PushRecv(storedbuffer.data(), storedbuffer.size());
			realPacketSize += storedbuffer.size();

			int16 fracount = orderedPacket->GetFragCount();
			if (fracount > 1)
				realPacketSize += sizeof(FragmentHeader) * fracount + sizeof(PacketHeader) * (fracount - 1); // (fracount - 1) -> 이미 storedbuffer.size에는 한개의 PacketHeader 사이즈가 포함된 값이기때문에 빼준다

			allSize += realPacketSize;
			//cout << "realPacketSize : " << realPacketSize << endl;
			_orderedPacketQueue.pop();
		}
		//cout << "allSize : " << allSize << " | Befroe RWind : " << dm->GetRWind();
		dm->MakeSpaceRWind(allSize); // RecvBuffer에서 Logic 처리로 옮겨졌기때문에 RWind의 크기를 해당 패킷 사이즈 만큼 다시 넓혀줘야 받을 수 있음
		dm->AddTotal_Recv_RWind(allSize);
		//cout <<" | " << "After RWind : " << dm->GetRWind() << endl;
		//cout << "dm->GetTotal_Recv_RWind() : " << dm->GetTotal_Recv_RWind() << endl;
		GTransportControl.OnPushRWind(_owner->client_Id, allSize, dm->GetTotal_Recv_RWind());
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

void QoSPlayer::InitSendQueuesOutStand()
{
	_sendQueues[QoS::Channel::RO]._maxOutStandBudget = RO_OUTSTAND;
	_sendQueues[QoS::Channel::URO]._maxOutStandBudget = URO_OUTSTAND;

}

void QoSShard::MakeQoSPlayer(HostRef object, int32 client_Id, int32 rate, int32 burst)
{
	WRITE_LOCK;

	//TODO 만약 같은 id를 사용하기 원하는 Player가 나왔을때 그냥 무시할 것인지 아님 emplace + check을 고려할 것인지 생각
	_qosPlayers[client_Id] = MakeShared<QoSPlayer>(object, this, rate, burst);
}

void QoSShard::ErasePlayer(int32 client_Id)
{
	WRITE_LOCK;
	/*shared_ptr<QoSPlayer> player;
	auto iterator = _qosPlayers.find(playerId);
	if (iterator != _qosPlayers.end())
		player = iterator->second;*/
	_qosPlayers.erase(client_Id);
	

}

//void QoSShard::PushSend(int32 playerid, SendBufferRef packet)
//{
//	shared_ptr<QoSPlayer> qosplaayer;
//	
//	{
//		READ_LOCK;
//		auto iter  = _qosPlayers.find(playerid);
//
//
//		if (iter == _qosPlayers.end())
//			return;
//
//		
//		qosplaayer = iter->second;
//	}
//
//	
//	qosplaayer->PushSend(packet);
//}

void QoSShard::PushSend(int32 playerid, shared_ptr<vector<SendBufferRef>> packet)
{
	shared_ptr<QoSPlayer> qosplaayer;

	{
		READ_LOCK;
		auto iter = _qosPlayers.find(playerid);


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

void QoSCore::OnRecv(int32 SeqNum, int32 client_Id, BYTE* buffer, int32 size)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	if (header->channel == QoS::Channel::RO) // Reliable Ordered
	{
		
		OnOrderedRecv(SeqNum, client_Id, buffer, size);
	
	}
	else // UnReliable Ordered or RFCT
	{
		PushRecv(client_Id, buffer, size);
	}
}

//void QoSCore::PushSend(int32 client_Id, SendBufferRef packet)
//{
//	QoSShard* shard = GetShard(client_Id);
//	shard->PushSend(client_Id,  packet);
//	
//}

void QoSCore::PushSend(int32 client_Id, shared_ptr<vector<SendBufferRef>> packet)
{
	QoSShard* shard = GetShard(client_Id);
	shard->PushSend(client_Id, packet);
}

void QoSCore::PushRecv(int32 client_Id, BYTE* buffer, int32 size)
{
	QoSShard* shard = GetShard(client_Id);
	shard->PushRecv(client_Id, buffer, size);

}

void QoSCore::OnOrderedRecv(int32 SeqNum, int32 client_Id, BYTE* buffer, int32 size)
{
	QoSShard* shard = GetShard(client_Id);
	shard->OnOrderedRecv(SeqNum, client_Id, buffer, size);
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
	while (running.load())
	{
		shared_ptr<QoSPlayer> _player = PopSendReadyQueue();
		if (!_player) {
			//TODO Wait or Do Something
			QoSShard* _busyShard = GQoS->GetBusyShard_SEND();

			if (!_busyShard)
			{
				std::unique_lock<mutex> _lock(GQoS->GetSendMutex());
				GQoS->GetSendCV().wait(_lock, [&]() { return !Empty_SendReadyQueue() || GQoS->GetBusyShard_SEND() || !running.load(); });
				if (!running.load())
					return;
				//cout << "QoSShard::DoSendWork()" << endl;
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

void QoSShard::Stop()
{
	running.exchange(false); 
	GQoS->GetSendCV().notify_all();
}

bool QoSShard::Empty_RecvReadyQueue()
{
	READ_LOCK_IDX(1);
	return _recvReadyQueue.empty();
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

bool QoSShard::Empty_SendReadyQueue()
{
	READ_LOCK_IDX(2); 
	return _sendReadyQueue.empty();
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

{
	GThreadManager->Launch([this]() {
		running.exchange(true);
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

void QoSPlayer::QoSSendQueue::Push(const shared_ptr<SavedSendPacket>& rhs, uint16 priority)
{
	ASSERT_CRASH(priority >= 0 && priority <QoS::Priority::PRIORTY_NUM);

	_queue[priority].push(rhs);
	
}

void QoSPlayer::QoSSendQueue::UpdateOutStandBudget()
{
#ifdef _DEBUG
	ASSERT_CRASH(_maxOutStandBudget > 0);
#endif

	if (_recentOutStandBudgetUpdateDate == 0) // when first call func
	{
		_recentOutStandBudgetUpdateDate = UTime::GetNow();
		_outStandBudget = _maxOutStandBudget;
		return;
	}
	double secRate = (double)(UTime::GetNow() - _recentOutStandBudgetUpdateDate) / 1'000'000;
#ifdef _DEBUG
	if (secRate < 0)
		CRASH("secRate < 0");
#endif
	int64 newBudget = _outStandBudget + _maxOutStandBudget * secRate;

	_outStandBudget = _maxOutStandBudget < newBudget ? _maxOutStandBudget : newBudget;
}

bool QoSPlayer::QoSSendQueue::CheckHas_OutStandBudget(int32 packetSize)
{
	if (_outStandBudget - packetSize < 0)
		return false;
	return true;
}

void QoSPlayer::QoSSendQueue::Use_OutStandBudget(int32 packetSize)
{
#ifdef _DEBUG
	ASSERT_CRASH(packetSize > 0);
	ASSERT_CRASH(packetSize <= _outStandBudget);
#endif
	_outStandBudget -= packetSize;
}

void QoSPlayer::QoSRecvQueue::Push(const shared_ptr<SavedRecvPacket>& rhs, uint16 priority)
{
	ASSERT_CRASH(priority >= 0 && priority <QoS::Priority::PRIORTY_NUM);

	_queue[priority].push(rhs);

}

