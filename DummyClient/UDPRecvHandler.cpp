#include "pch.h"
#include "UDPRecvHandler.h"
#include "ThreadManager.h"

UDPRecvHandler::UDPRecvHandler(QoSShard* _ownerShard)
	: _shard(_ownerShard)
{
	CreateAsyncWorkThread();
}

UDPRecvHandler::~UDPRecvHandler()

{
	StopThread();
}


void UDPRecvHandler::CreateAsyncWorkThread()
{
	
	_ASSERT(_shard != nullptr);

	GThreadManager->Launch([this]() {
		_continue = true;
		DOWork();
		});
}

void UDPRecvHandler::DOWork()
{
	while (_continue)
	{
		shared_ptr<QoSPlayer> _player = _shard->PopRecvReadyQueue();
		if (!_player) {
			//TODO Wait or Do Something
			QoSShard* _busyShard = GQoS->GetBusyShard_RCV();
			
			if (!_busyShard)
			{
				continue;
				//this_thread::sleep_for(0ms);
			}
			_player = _busyShard->PopRecvReadyQueue();
			if (!_player)
			{
				//SleepTillGetSignal();
				continue;
				
			}
		}
		
		HandleRecvPacket(_player);
	}
}

void UDPRecvHandler::HandleRecvPacket(std::shared_ptr<QoSPlayer> _Player)
{
	
	shared_ptr<SavedRecvPacket> savePacket; //기아 현상 해결해야함
	

	int coin = 32;

	PlayerRef player = static_pointer_cast<Player>(_Player->GetOwner());

	do
	{
		savePacket = _Player->PopRecv_RO();
		if (!savePacket) // 더이상 RO RecvQueue에 Packet이 존재하지 않음
		{
			
			break;
		}
		
		ServerPacketHandler::HandlePacket(player->ownerSocket, player->netAddress, savePacket->_buffer, savePacket->_size);
				
	} while (coin--);
	//coin = 3;

	//coin = 32;
	do
	{
		savePacket = _Player->PopRecv_URO();
		if (!savePacket) // 더이상 URO RecvQueue에 Packet이 존재하지 않음
		{
			
			break;
		}
		ServerPacketHandler::HandlePacket(player->ownerSocket, player->netAddress, savePacket->_buffer, savePacket->_size);

	} while (coin--);
	//coin = 2;

	savePacket = _Player->PopRecv_RFCT();
	if (savePacket) // RFCT Packet이 업데이트 됨
	{
		ServerPacketHandler::HandlePacket(player->ownerSocket, player->netAddress, savePacket->_buffer, savePacket->_size);
	}
	
	
	_Player->ResetRecvReady();
}
void UDPRecvHandler::StopThread()
{
	_continue = false;
}
//void UDPRecvHandler::SleepTillGetSignal()
//{
//	unique_lock<mutex> _uq_lock(_mutex);
//	_shard->PushSleepWorker(_cv);
//	_cv.wait(_uq_lock);
//}
//void UDPRecvHandler::SetSignal()
//{
//	_cv.notify_one();
//}
//
//void UDPRecvHandler::Push(const JobFunc& _job)
//{
//	JobQueue.push(_job);
//}
//
//JobFunc UDPRecvHandler::Pop()
//{
//	
//	JobFunc func;
//	//Lock 사용 X -> pop하는 thread가 단일 thread에서 async하게 돌아갈 예정(push하는 thread랑 다르지만 그쪽에서는 push만 하기때문에 이론상 UB x)
//	if (!JobQueue.empty())
//	{
//		func = JobQueue.front();
//		JobQueue.pop();
//	}
//	//else if (!JobQueue.empty())
//	//{
//	//	//cout << "LowJobQueue Size : " << lowJobQueue.size() << endl;
//	//	func = lowJobQueue.front();
//	//	lowJobQueue.pop();
//
//	//}
//	return func;
//}
//


//
//void UDPRecvHandler::PushSend(JobFunc job, int priority)
//{
//	WRITE_LOCK;
//	if (priority == QoSCore::HIGH)
//	{
//		highJobQueue.push(job);
//	}
//	else if (priority == QoSCore::LOW)
//	{
//		lowJobQueue.push(job);
//	}
//}
//
//void UDPRecvHandler::DOJob()
//{
//	JobFunc func = Pop();
//	if (func == nullptr)
//		return;
//	func();
//}
//

