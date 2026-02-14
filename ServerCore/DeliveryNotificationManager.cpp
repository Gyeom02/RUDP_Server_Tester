#include "pch.h"
#include "DeliveryNotificationManager.h"
#include "Host.h"
//#define TEST_PRINT
//#define TEST_PRINT2
DeliveryNotificationManager::DeliveryNotificationManager()
	: _recvWindow(0)
{
}

DeliveryNotificationManager::~DeliveryNotificationManager()
{
}

bool DeliveryNotificationManager::CheckPacketChannel(PacketHeader* header)
{
	switch (header->channel)
	{
	case QoS::Channel::RO:
		if (ProcessSequenceNumber(header) == false)
			return false;
		break;
	case QoS::Channel::URO:
		if (ProcessSequenceNumber_URO(header->sn) == false)
			return false;
		break;
	case QoS::Channel::RPCT:
		break;
	}

	return true;

}
//InFlightPacketPtr DeliveryNotificationManager::WriteSeqeuenceNumber(SOCKET object, NetAddress netAddr, SendBufferRef sendBuffer)
//{
//	
//	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
//	PacketSequenceNumber sequenceNumber = mNextOutgoingSequenceNumber++;
//	header->sn = sequenceNumber.GetSN(); // 패킷 헤더에 SequenceNumber 부착
//
//	++mDispatchedPacketCount;
//
//	WRITE_LOCK;
//
//	mInFlightPackets.emplace_back(MakeShared<InFlightPacket>(object, netAddr, sequenceNumber, sendBuffer));
//	
//	return mInFlightPackets.back();
//}
//InFlightPacketPtr DeliveryNotificationManager::WriteSeqeuenceNumber(SendBufferRef sendBuffer)
//{
//	
//	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
//	PacketSequenceNumber sequenceNumber = mNextOutgoingSequenceNumber++;
//	if(header->retransnum <= 0) // 처음 전송일때만 sn 초기화
//		header->sn = sequenceNumber.GetSN(); // 패킷 헤더에 SequenceNumber 부착
//
//	++mDispatchedPacketCount;
//
//	WRITE_LOCK;
//
//	HostRef owner;
//	if (owner = _weakOwner.lock());
//		mInFlightPackets.emplace_back(MakeShared<InFlightPacket>(owner, sequenceNumber, sendBuffer));
//	
//	return mInFlightPackets.back();
//}

void DeliveryNotificationManager::WriteSeqeuenceNumber(shared_ptr<vector<SendBufferRef>> sendBuffers)
{
	HostRef owner = _weakOwner.lock();
	uint32 startSN = 0;
	//int32 bufferNum = sendBuffer->size();
	uint32 firstSN = 0;
	//WRITE_LOCK_IDX(InFlightPacket_LOCK);

	PacketHeader* firstheader = reinterpret_cast<PacketHeader*>((*sendBuffers)[0]->Buffer());
	if (firstheader->priority != QoS::Priority::RESEND)
	{
		startSN = mNextOutgoingSequenceNumber.fetch_add(sendBuffers->size());
		
	}
	for (int32 i = 0; i < sendBuffers->size(); i++)
	{
		SendBufferRef sendBuffer = (*sendBuffers)[i];
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		//cout << "bufferNum : " << bufferNum << endl;
		//if(firstHeader->retransnum <= 0)// 처음 전송일때만 sn 초기화 
		//{
		//	startSN = mNextOutgoingSequenceNumber.fetch_add(1);
		//	
		//}

		//PacketHeader* header = reinterpret_cast<PacketHeader*>((*sendBuffer)[i]->Buffer());
		InFlightPacketPtr inflightPacket;
		if (header->priority != QoS::Priority::RESEND)  // 처음 전송일때만 sn 초기화
		{
			
			
			//PacketSequenceNumber sequenceNumber = mNextOutgoingSequenceNumber.fetch_add(1);
			
			header->sn = startSN++; // 패킷 헤더에 SequenceNumber 부착
			inflightPacket = MakeShared<InFlightPacket>(owner, header->sn, sendBuffer);
			
			StoreInFlightPacketFromSN(header->sn, inflightPacket);
			//mInFlightPacketsSN.Push(header->sn);

			if (header->bFragment == 1) //Fragment Packet이다
			{
				FragmentHeader* fragHeader = reinterpret_cast<FragmentHeader*>(&header[1]);
				if (fragHeader->index == 0) // 첫번째 fragment이다
				{
					firstSN = header->sn;
				}
	
				fragHeader->first_sn = firstSN;
			}
			inflightPacket->InitSendTime();
		}
		else
		{
			inflightPacket = FindInFlightPacketFromSN(header->sn);
			if (!inflightPacket)
				return;
				//	cout << "WriteSeqeuenceNumber 110 layer" << endl;
				
			
		//	header->sn = inflightPacket->GetSequenceNumber().GetSN();
			inflightPacket->UpdateRecentSendTime();
			
		}


		//else cout << "header->sn : " << header->sn << endl;
		++mDispatchedPacketCount;

		/*WRITE_LOCK_IDX(InFlightPacket_LOCK);
		*/
		GTransportControl.PushRTOQueue({ inflightPacket, inflightPacket->GetRecentSendTime() + owner->GetRTTManager().GetRTO() });
		//mInFlightPacketsSN.Push(header->sn);

		
	}
	//return mInFlightPackets.back();
}

void DeliveryNotificationManager::ProcessAcks(uint32 start, uint32 count, bool hasCount, uint32 curExpectedSN)
{
	AckRange ackRange;
	ackRange.AckRead(start, count);
	//cout << "ackRange.AckRead(start, count) : start : " << start << " | count : "<< count << endl;
	PacketSequenceNumber nextAckdSequenceNumber = ackRange.GetStart();
	uint32 onePastAckdSequenceNumber = nextAckdSequenceNumber.GetSN() + ackRange.GetCount();

	

	while (nextAckdSequenceNumber.GetSN() < onePastAckdSequenceNumber)
	{
		int success_flag = -1;
		InFlightPacketPtr nextInFlightPacket;
		{
			
			/*if (mInFlightPackets.empty())
				break;*/
			//nextInFlightPacket = mInFlightPackets.front();
			//패킷의 시퀀스 번호가 확인응답의 시퀀스 번호보다 작으면 확인응답을 받지 못한 것이므로 아마 누락되었을 것임
			//PacketSequenceNumber nextInFlightPacketSequenceNumber = nextInFlightPacket->GetSequenceNumber();
			uint32 expectedAckSN = _curExpectedAckSN.load();

			if (expectedAckSN < nextAckdSequenceNumber.GetSN())
			{
				//사본을 만든다음, 목록에서 일단 제거함
				//핸들링 도중 살아있는 패킷이 무엇인지 찾아볼 때 이 패킷에 보여서는 안되기 때문이다.
				//auto copyOfInFlightPacket = nextInFlightPacket;
				if (!CheckValidAckSN(nextAckdSequenceNumber.GetSN())) // Handle 가능한 SN이 아님
				{

					CRASH("!CheckValidAckSN(nextAckdSequenceNumber.GetSN()");
					//cout << "nextInFlightPacketSequenceNumber.GetSN() + mInFlightPackets.size() - 1 < nextAckdSequenceNumber.GetSN()" << endl;

					//break;
				}
				else
				{
					
					nextInFlightPacket = HandleAck(nextAckdSequenceNumber.GetSN()); // Map에서 제거함과 동시에 Shared Ptr 얻음
					//nextInFlightPacket->GotAck(); // Ack 받았음을 처리
					//nt32 index = nextAckdSequenceNumber.GetSN() - nextInFlightPacketSequenceNumber.GetSN();
					//nextInFlightPacket = mInFlightPackets[index];
					//ASSERT_CRASH(nextInFlightPacket);

					/*auto iter = mInFlightPackets.begin() + index;
					mInFlightPackets.erase(iter);*/
					++nextAckdSequenceNumber;
					if (!nextInFlightPacket)
					{
						//cout << "!nextInFlightPacket" << endl;
#ifdef TEST_PRINT
						cout << "end() Recv Ack Success ID : " << _weakOwner.lock()->client_Id << " | expected sn : " << _curExpectedAckSN.load() << " |  Handled sn : " << nextAckdSequenceNumber.GetSN() - 1 << " | recviver expected sn : " << curExpectedSN << endl;
#endif
						continue;
					}
					//mSequenceNotMatchedCount++;
					
					success_flag = 1;
					
				}
				
				
				
				
				//cout << nextInFlightPacketSequenceNumber.GetSN() << " < " << nextAckdSequenceNumber.GetSN() << endl;
			}
			
			else if (expectedAckSN == nextAckdSequenceNumber.GetSN())
			{
				//	cout << "DeliverySuccess" << endl;
				
				nextInFlightPacket = HandleAck(_curExpectedAckSN.fetch_add(1));

				++nextAckdSequenceNumber;
				if (!nextInFlightPacket)
				{
					//cout << "!nextInFlightPacket SN : " << nextAckdSequenceNumber.GetSN() <<  endl;
#ifdef TEST_PRINT
					cout << "end() Recv Ack Success ID : " << _weakOwner.lock()->client_Id << " | expected sn : " << _curExpectedAckSN.load() - 1 << " |  Handled sn : " << nextAckdSequenceNumber.GetSN() - 1 << " | recviver expected sn : " << curExpectedSN << endl;
#endif
					continue;
				}
				//mInFlightPackets.pop_front();
				
				success_flag = 1;
				
			}

			else if (expectedAckSN > nextAckdSequenceNumber.GetSN())
			{
				//일부 응답이 어떤 연유에선지 제거되었음(시간 초과 가능성)
				//나머지를 계속하여 검사함
			//	cout << "nextInFlightPacketSequenceNumber > nextAckdSequenceNumber" << endl;
				nextInFlightPacket = HandleAck(nextAckdSequenceNumber.GetSN());
				++nextAckdSequenceNumber;
				if (nextInFlightPacket)
					success_flag = 1;
				else
					continue;
				//nextAckdSequenceNumber = expectedAckSN;
				
			}
		}
		
		//if (success_flag == 1) //
		//	HandlePacketDeliveryFailure(nextInFlightPacket);
		if (success_flag == 1) //Ack 성공
		{
#ifdef TEST_PRINT
			cout << "Recv Ack Success ID : " << _weakOwner.lock()->client_Id << " | expected sn : " << _curExpectedAckSN.load() - 1 << " |  Handled sn : " << nextAckdSequenceNumber.GetSN()  - 1<< " | recviver expected sn : " << curExpectedSN  << endl;
#endif
			HandlePacketDeliverySuccess(nextInFlightPacket);
		}

		else if (success_flag == 2) //
			continue;
		else
			ASSERT_CRASH(false);


	}
	//cout << "UpdateExpectedAckSN : " << _weakOwner.lock()->client_Id << " | expected sn : " << _curExpectedAckSN.load() - 1 << " |  Handled sn : " << nextAckdSequenceNumber.GetSN() - 1 << " | recviver expected sn : " << curExpectedSN << endl;

	UpdateExpectedAckSN(curExpectedSN);
}

void DeliveryNotificationManager::HandlePacketDeliveryFailure(const InFlightPacketPtr& inFlightPacket)
{
	++mResendPacketCount;
	inFlightPacket->HandleDeliveryFailure(shared_from_this());
}

void DeliveryNotificationManager::HandlePacketDeliverySuccess(const InFlightPacketPtr& inFlightPacket)
{
	++mDeliveredPacketCount;
	//cout << "DeliveredCount : " << mDeliveredPacketCount << endl;
	inFlightPacket->HandleDeliverySuccess(shared_from_this());
}

bool DeliveryNotificationManager::ProcessSequenceNumber(PacketHeader* header)
{
	int32 size = 0;
	uint32 SN = header->sn;
	FragmentHeader* fragHeader = nullptr;

	if (header->retransnum > 0)
		AddPendingAck(SN);
	else
		AddPendingAck(SN, header->sent_timestamp);


	uint32 expectedSN = _recvWindow.GetExpectedSqeNum();


	if (SN < expectedSN) //기다리고 있었던 수신 패킷 세퀀스 넘버가 아님 조용히 넘김
	{
		//cout << "ID : " << header->client_Id << " | SN.GetSN() < mNextExpectedSequenceNumber : " << SN << " < " << expectedSN << endl;

		return false;
	}

	//if (header->bFragment == 1) //Fragment Packet인지에 따라 size 값 세팅
	//{
	//	fragHeader = reinterpret_cast<FragmentHeader*>(&header[1]);
	//	//cout << "fragHeader->original_size : " << fragHeader->original_size << endl;

	//	size += fragHeader->original_size;
	//	size += (sizeof(FragmentHeader) * fragHeader->frag_count) + (sizeof(PacketHeader) * fragHeader->frag_count);
	//}
	//else
	//{
	//	size = header->size;
	//}

	if (!_recvWindow.CheckRecved(SN)) // 중복 Seq
	{

		//cout << "!_recvWindow.CheckRecved(SN.GetSN()) | " << "ID : " << header->client_Id << " | Recved SN : " << SN << " | Size : " << size << " | Expected SN : " << expectedSN <<endl;

		return false;
	}

	_recvWindow.TryRecv(SN); 

	if (!_recvWindow.ReduceSpaceRWind(header->size)) // 로직상 발생하면 안되는 상황
	{
		CRASH("Reciver has no RWind to Recv Packet");
	}
	//
//	if (!_recvWindow.IsSpaceExistToRecv(header->size))
//	{
//		cout << "ProcessSequenceNumber Out Of RWind : " << GetRWind() << " < " << header->size << endl;
//
//		return false;
//	}
//	if (header->bFragment != 1) // rwind valid size check
//	{
//		
//		
//		if (!_recvWindow.CheckSize(SN, size))
//		{
//			cout << "!_recvWindow.CheckSize(SN, size) : " << "Packet Size : " << size << endl;
//
//			return false;
//		}
//		_recvWindow.TryRecv(SN, size);  
//	}
//	else
//	{
//		if (!_recvWindow.CheckAndTryFrag(header, fragHeader, size))
//		{
//			cout << "!_recvWindow.CheckAndTryFrag" << endl;
//#ifdef _DEBUG
//			if (size >= 1503315811)
//			{
//				CRASH("size >= 1503315811");
//			}
//#endif
//			return false;
//		}
//	//	_recvWindow.TryFrag(SN, header->size); 
//		//_recvWindow.TryRecv(SN, header->size);
//	}
//	_recvWindow.ReduceSpaceRWind(header->size);
	
	
	//cout << "  SN.GetSN() : " << SN.GetSN() << endl;
	//
	//if (SN > mNextExpectedSequenceNumber)
	//{
	//	//cout << "AddPendingAck(SN);" << endl;
	//	AddPendingAck(SN);
	//	//return true;
	//	
	//}

	//if (SN == expectedSN) //예상하던 수신 패킷 세퀀스넘버가 맞음
	//{
	//	mNextExpectedSequenceNumber += 1;

	//	//cout << "AddPendingAck(SN);" << endl;
	//	//return true;
	//}
#ifdef TEST_PRINT2
	cout << "Recv Success ID : " << _weakOwner.lock()->client_Id  << " |  Handled sn : " << SN << " | recviver expected sn : " << _recvWindow.GetExpectedSqeNum() << endl;
#endif
	return true;

}

void DeliveryNotificationManager::AddPendingAck(PacketSequenceNumber SN, uint64 timestamp)
{
	bool bexpected = false;
	
	WRITE_LOCK_IDX(Ack_LOCK);
	if (mPendingAcks.size() == 0 || !mPendingAcks.back().ExtendIfShould(SN))
	{
		//cout << "if (mPendingAcks.size() == 0 || !mPendingAcks.back().ExtendIfShould(SN))" << endl;
		mPendingAcks.emplace_back(SN.GetSN(), timestamp);
	}

	if (bInsertAckReadyQueue.compare_exchange_strong(bexpected, true))
	{
		HostRef ownerHost;
		if (ownerHost = _weakOwner.lock())
		{
			GTransportControl.PushHostAckReady(ownerHost);
			GTransportControl.GetLazyAssist()._jobCv.notify_one();
			//cout << " ownerHost == _owner.lock()" << endl;
		}
		else
		{
			CRASH("Host class Missed Call InitDeliveryManager");
			/*bInsertAckReadyQueue.exchange(false);
			cout << " ownerHost != _owner.lock()" << endl;*/
		}
	}
}

bool DeliveryNotificationManager::WritePendingAcks(OUT uint32& start, OUT int32& count, OUT bool& hasCount, OUT uint64& rtt_stamp)
{
	WRITE_LOCK_IDX(Ack_LOCK);
	bool hasAcks = (mPendingAcks.size() > 0);
	if (hasAcks)
	{
		mPendingAcks.front().AckWrite(start, count, hasCount, rtt_stamp);
		mPendingAcks.pop_front();
		return true;
	}
	return false;
}

void DeliveryNotificationManager::ProcessTimeOutPackets()
{
	//WRITE_LOCK;
	//int32 token = 10;
	
	

	

	while (true)
	{
		uint32 nextInFlightPacketSN;
		InFlightPacketPtr nextInFlightPacket;
		int32 handleFlag = -1; // 1 = success | 2 = fail
		
		//WRITE_LOCK_IDX(InFlightPacket_LOCK);
		if (mInFlightPacketsSN.Empty())
			return;
			
			
		nextInFlightPacketSN = mInFlightPacketsSN.Front();

		nextInFlightPacket = FindInFlightPacketFromSN(nextInFlightPacketSN);
		//cout << "1" << endl;
		if (!nextInFlightPacket) // 이미 Ack 처리한 패킷임
		{
			mInFlightPacketsSN.Pop();
			continue;
		}

		uint64 now = UTime::GetNow();

		if ((now - nextInFlightPacket->GetRecentSendTime()) / 1'000 > TIMEOUT)
		{
				
			//if (nextInFlightPacket->GetTransmissionData()->IsGotAck()) // Ack Packet을 받은 패킷임
			//{
			//	mInFlightPackets.pop_front();
			//}
			
			PacketHeader* header = reinterpret_cast<PacketHeader*>(nextInFlightPacket->GetTransmissionData()->Buffer());
			if (header->sn < _curExpectedAckSN.load()) //_curExpectedAckSN보다 작다는건 이미 처리된 SN을 가진 패킷임으로 성공처리함(그저 Ack을 못받았을 뿐)
			{
				//cout << "2" << endl;
				handleFlag = 1;

				mInFlightPacketsSN.Pop();
				//continue;
			}
			else
			{
				if (!nextInFlightPacket->Check_CoolTime_ReSend())
					return;
				//cout << "3" << endl;
				
				//cout << "4" << endl;
				mTimeOutCount++;
				handleFlag = 2;
				//PacketHeader* header = reinterpret_cast<PacketHeader*>(nextInFlightPacket->GetTransmissionData()->Buffer());
			//	cout << " TimeOutPacket ID : " << header->id << " | SN : " << header->sn << endl;

				//mInFlightPacketsSN.Pop();
				return;
			}
				
		}
		else
		{
			//cout << "5" << endl;
			return; //이후 다음 패킷부터는 초과가 아님(시간순서대로 넣어져있기 때문이다)
		}
		
		switch (handleFlag)
		{
		case 1:
			HandleAck(nextInFlightPacketSN);
			HandlePacketDeliverySuccess(nextInFlightPacket);
			break;
		case 2:
			HandlePacketDeliveryFailure(nextInFlightPacket);
			break;
		default:
			break;
		}
	}
}

bool DeliveryNotificationManager::ProcessSequenceNumber_URO(PacketSequenceNumber SN)
{
	if (SN.GetSN() >= mNextExpectedSequenceNumber_URO) //예상하던 수신 패킷 세퀀스넘버가 맞음
	{
		mNextExpectedSequenceNumber_URO = SN.GetSN() + 1;
		//AddPendingAck(SN);
		return true;
	}

	else if (SN.GetSN() < mNextExpectedSequenceNumber_URO) //기다리고 있었던 수신 패킷 세퀀스 넘버가 아님 조용히 넘김
	{
		return false;
	}

	return false;
}

bool DeliveryNotificationManager::WriteSeqeuenceNumber_URO(SendBufferRef sendBuffer)
{
	
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());

	if (header == nullptr)
		return false;
	PacketSequenceNumber sequenceNumber = mNextOutgoingSequenceNumber_URO++;
	header->sn = sequenceNumber.GetSN(); // 패킷 헤더에 SequenceNumber 부착

	//++mDispatchedPacketCount;
	
	return true;
}

bool DeliveryNotificationManager::WriteSeqeuenceNumber_URO(shared_ptr<vector<SendBufferRef>> sendBuffer)
{
	for (int32 i = 0; i < (*sendBuffer).size(); i++)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>((*sendBuffer)[i]->Buffer());

		if (header == nullptr)
			return false;
		PacketSequenceNumber sequenceNumber = mNextOutgoingSequenceNumber_URO++;
		header->sn = sequenceNumber.GetSN(); // 패킷 헤더에 SequenceNumber 부착

		//++mDispatchedPacketCount;
	}
	return true;
}

bool DeliveryNotificationManager::CheckHostAckEmpty()
{
	WRITE_LOCK_IDX(Ack_LOCK)
	if (mPendingAcks.empty()) //비어있음을 확인
	{
		bInsertAckReadyQueue.exchange(false);
		return true;
	}
	return false;
}

InFlightPacketPtr DeliveryNotificationManager::FindOldestInFlightPacket()
{
	auto ptr = FindInFlightPacketFromSN(_curExpectedAckSN.load());
	if (ptr == nullptr)
	{
		return nullptr;
	}
	return ptr;
}

InFlightPacketPtr DeliveryNotificationManager::FindInFlightPacketFromSN(uint32 sn)
{
	READ_LOCK_IDX(Map_LOCK);
	auto iter = mSnToInFlightPacketMap.find(sn);
	if (iter == mSnToInFlightPacketMap.end())
	{
		return nullptr;
	}
	PacketHeader* header = reinterpret_cast<PacketHeader*>(iter->second->GetTransmissionData()->Buffer());
	//if (header->sn != sn) {
	//	//cout << "A" << endl;
	//	header->sn = sn;
	//}
	return iter->second;
}

void DeliveryNotificationManager::StoreInFlightPacketFromSN(uint32 sn, InFlightPacketPtr inflightPacket)
{
	WRITE_LOCK_IDX(Map_LOCK);
	//cout << "StoreInFlightPacketFromSN : " << sn << endl;
	/*if (mSnToInFlightPacketMap.find(sn) != mSnToInFlightPacketMap.end())
		CRASH("!");*/
	mSnToInFlightPacketMap.emplace(sn, inflightPacket);
	
	//	cout << "StoreInFlightPacketFromSN Failed SN : " << sn << endl;
	//else cout << "StoreInFlightPacketFromSN SN : " << sn << endl;
}

InFlightPacketPtr DeliveryNotificationManager::EraseInFlightPacketFronSN(uint32 sn)
{
	
	WRITE_LOCK_IDX(Map_LOCK);
	//cout << "EraseInFlightPacketFronSN : " << sn << endl;
	auto iter = mSnToInFlightPacketMap.find(sn);
	if (iter == mSnToInFlightPacketMap.end())
		return nullptr;
	//ASSERT_CRASH(iter != mSnToInFlightPacketMap.end());

	InFlightPacketPtr inflightPacket = iter->second;
	mSnToInFlightPacketMap.erase(iter);

	return inflightPacket;
}


bool DeliveryNotificationManager::CheckValidAckSN(uint32 sn)
{
	if (sn >= _curExpectedAckSN.load() + RUDPWIND::SN_RANGE_HALF)
		return false;
	return true;
}

InFlightPacketPtr DeliveryNotificationManager::HandleAck(uint32 sn)
{
	InFlightPacketPtr nextInFlightPacket = EraseInFlightPacketFronSN(sn);// Map에서 제거함과 동시에 Shared Ptr 얻음
	//InFlightPacketPtr nextInFlightPacket = FindInFlightPacketFromSN(sn);
	if (!nextInFlightPacket)
		return nullptr;
	nextInFlightPacket->GotAck();
	return nextInFlightPacket;
}

void DeliveryNotificationManager::UpdateExpectedAckSN(uint32 newSN)
{
	if (newSN > _curExpectedAckSN.load())
		_curExpectedAckSN.store(newSN);
}

bool AckRange::ExtendIfShould(PacketSequenceNumber SN)
{
	if (SN.GetSN() == mStart + mCount)
	{
		++mCount;
		return true;
	}
	else
	{
		return false;
	}
}

void AckRange::AckWrite(OUT uint32& start, OUT int32& count, OUT bool& hasCount, OUT uint64& rtt_stamp)
{
	start = mStart;
	hasCount = mCount > 1;
	if (hasCount)
	{
		int32 countMinusOne = mCount - 1; // 기준 값 제외 나머지 패킷 수
		uint8 countToAck = countMinusOne > 255 ? 255 : static_cast<uint8>(countMinusOne);
		count = static_cast<int32>(countToAck);
	}
	rtt_stamp = this->rtt_stamp;
	//netAddr = netAddress;
}

void AckRange::AckRead(uint32 start, int32 count)
{
	mStart = start;

	bool hasCount = count > 0;
	if (hasCount)
	{
		mCount = count + 1;
	}
	else mCount = 1; // default

	//netAddress = netAddr;
}
