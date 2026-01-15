#include "pch.h"
#include "DeliveryNotificationManager.h"
#include "Host.h"

DeliveryNotificationManager::DeliveryNotificationManager()
	: mNextExpectedSequenceNumber(0), _recvWindow(mNextExpectedSequenceNumber)
{
}

DeliveryNotificationManager::~DeliveryNotificationManager()
{
}

bool DeliveryNotificationManager::CheckPacketChannel(int16 channel, uint32 sn, int32 size)
{
	switch (channel)
	{
	case QoS::Channel::RO:
		if (ProcessSequenceNumber(sn, size) == false)
			return false;
		break;
	case QoS::Channel::URO:
		if (ProcessSequenceNumber_URO(sn) == false)
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

InFlightPacketPtr DeliveryNotificationManager::WriteSeqeuenceNumber(shared_ptr<vector<SendBufferRef>> sendBuffer)
{
	HostRef owner = _weakOwner.lock();
	uint32 startSN;
	int32 bufferNum = sendBuffer->size();
	PacketHeader* firstHeader = reinterpret_cast<PacketHeader*>((*sendBuffer)[0]->Buffer());
	//cout << "bufferNum : " << bufferNum << endl;
	if(firstHeader->retransnum <= 0)// 처음 전송일때만 sn 초기화 
	{
		startSN = mNextOutgoingSequenceNumber.fetch_add(bufferNum);
		
	}
	for (int32 i = 0; i < sendBuffer->size(); i++)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>((*sendBuffer)[i]->Buffer());
		InFlightPacketPtr inflightPacket = MakeShared<InFlightPacket>(owner, header->sn, (*sendBuffer)[i]);
		if (header->retransnum <= 0)  // 처음 전송일때만 sn 초기화
		{
			PacketSequenceNumber sequenceNumber = startSN++;
			header->sn = sequenceNumber.GetSN(); // 패킷 헤더에 SequenceNumber 부착
			
		}
		StoreInFlightPacketFromSN(header->sn, inflightPacket);
		//else cout << "header->sn : " << header->sn << endl;
		++mDispatchedPacketCount;
		
		WRITE_LOCK_IDX(InFlightPacket_LOCK);

		if (owner);
			mInFlightPackets.emplace_back(inflightPacket);
	}
	return mInFlightPackets.back();
}

void DeliveryNotificationManager::ProcessAcks(uint32 start, uint32 count, bool hasCount)
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

					break;
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
					if (!nextInFlightPacket)
						continue;
					
					//mSequenceNotMatchedCount++;
					++nextAckdSequenceNumber;
					success_flag = 1;
					
				}
				
				
				
				
				//cout << nextInFlightPacketSequenceNumber.GetSN() << " < " << nextAckdSequenceNumber.GetSN() << endl;
			}
			
			else if (expectedAckSN == nextAckdSequenceNumber.GetSN())
			{
				//	cout << "DeliverySuccess" << endl;
				
				nextInFlightPacket = HandleAck(_curExpectedAckSN.fetch_add(1));
				if (!nextInFlightPacket)
					continue;
				//mInFlightPackets.pop_front();
				++nextAckdSequenceNumber;
				success_flag = 1;
				
			}

			else if (_curExpectedAckSN.load() > nextAckdSequenceNumber.GetSN())
			{
				//일부 응답이 어떤 연유에선지 제거되었음(시간 초과 가능성)
				//나머지를 계속하여 검사함
			//	cout << "nextInFlightPacketSequenceNumber > nextAckdSequenceNumber" << endl;
				nextAckdSequenceNumber = _curExpectedAckSN.load();
				success_flag = 2;
			}
		}
	
		//if (success_flag == 1) //
		//	HandlePacketDeliveryFailure(nextInFlightPacket);
		if (success_flag == 1) //Ack 성공
			HandlePacketDeliverySuccess(nextInFlightPacket);
		else if (success_flag == 2) //
			continue;
		else
#ifdef _DEBUG 
			ASSERT_CRASH(false);
#else		
			break;
#endif
	}
}

void DeliveryNotificationManager::HandlePacketDeliveryFailure(const InFlightPacketPtr& inFlightPacket)
{
	++mDroppedPacketCount;
	inFlightPacket->HandleDeliveryFailure(shared_from_this());
}

void DeliveryNotificationManager::HandlePacketDeliverySuccess(const InFlightPacketPtr& inFlightPacket)
{
	++mDeliveredPacketCount;
	//cout << "DeliveredCount : " << mDeliveredPacketCount << endl;
	inFlightPacket->HandleDeliverySuccess(shared_from_this());
}

bool DeliveryNotificationManager::ProcessSequenceNumber(PacketSequenceNumber SN, int32 size)
{
	
	if (SN.GetSN() < mNextExpectedSequenceNumber) //기다리고 있었던 수신 패킷 세퀀스 넘버가 아님 조용히 넘김
	{
		cout << "SN.GetSN() < mNextExpectedSequenceNumber : " << SN.GetSN() << " < " << mNextExpectedSequenceNumber << endl;

		return false;
	}
	if (!_recvWindow.CheckRecved(SN.GetSN())) // 중복 Seq
	{
		cout << "!_recvWindow.CheckRecved(SN.GetSN()) : " << "Recved SN : " << SN.GetSN() << "Expected SN : " <<_recvWindow.GetExpectedSqeNum() << endl;

		return false;
	}
	//
	if (!_recvWindow.IsSpaceExistToRecv(size)) // rwind valid size check
	{
		cout << "ProcessSequenceNumber Out Of RWind : " << GetRWind() << " < " << size << endl;

		return false;
	}

	_recvWindow.TryRecv(SN.GetSN()); _recvWindow.ReduceSpaceRWind(size);
	//cout << "  SN.GetSN() : " << SN.GetSN() << endl;
	
	if (SN.GetSN() > mNextExpectedSequenceNumber) //예상하던 수신 패킷 세퀀스넘버가 맞음
	{
		//cout << "AddPendingAck(SN);" << endl;
		AddPendingAck(SN);
		return true;
		
	}
	else if (SN.GetSN() == mNextExpectedSequenceNumber)
	{
		mNextExpectedSequenceNumber += 1;
		
		AddPendingAck(SN);

		//cout << "AddPendingAck(SN);" << endl;
		return true;
	}
	

	return true;

}

void DeliveryNotificationManager::AddPendingAck(PacketSequenceNumber SN)
{
	bool bexpected = false;
	
	WRITE_LOCK_IDX(Ack_LOCK);
	if (mPendingAcks.size() == 0 || !mPendingAcks.back().ExtendIfShould(SN))
	{
		//cout << "if (mPendingAcks.size() == 0 || !mPendingAcks.back().ExtendIfShould(SN))" << endl;
		mPendingAcks.emplace_back(SN.GetSN());
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

bool DeliveryNotificationManager::WritePendingAcks(OUT uint32& start, OUT int32& count, OUT bool& hasCount)
{
	WRITE_LOCK_IDX(Ack_LOCK);
	bool hasAcks = (mPendingAcks.size() > 0);
	if (hasAcks)
	{
		mPendingAcks.front().AckWrite(start, count, hasCount);
		mPendingAcks.pop_front();
		return true;
	}
	return false;
}

void DeliveryNotificationManager::ProcessTimeOutPackets()
{
	//WRITE_LOCK;
	WRITE_LOCK_IDX(InFlightPacket_LOCK);
	uint64 now = GetTickCount64();
	while (!mInFlightPackets.empty())
	{
		const auto& nextInFlightPacket = mInFlightPackets.front();
		if (now - nextInFlightPacket->GetTimeDispactched() > TIMEOUT)
		{
			if (nextInFlightPacket->GetTransmissionData()->IsGotAck()) // Ack Packet을 받은 패킷임
			{
				mInFlightPackets.pop_front();
			}
			else
			{ 
				mTimeOutCount++;
				//PacketHeader* header = reinterpret_cast<PacketHeader*>(nextInFlightPacket->GetTransmissionData()->Buffer());
			//	cout << " TimeOutPacket ID : " << header->id << " | SN : " << header->sn << endl;
				HandlePacketDeliveryFailure(nextInFlightPacket);
				mInFlightPackets.pop_front();
			}
		}
		else
			return; //이후 다음 패킷부터는 초과가 아님(시간순서대로 넣어져있기 때문이다)
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

InFlightPacketPtr DeliveryNotificationManager::FindInFlightPacketFromSN(uint32 sn)
{
	READ_LOCK_IDX(Map_LOCK);
	ASSERT_CRASH(mSnToInFlightPacketMap.find(sn) != mSnToInFlightPacketMap.end());

	return mSnToInFlightPacketMap[sn];
}

void DeliveryNotificationManager::StoreInFlightPacketFromSN(uint32 sn, InFlightPacketPtr inflightPacket)
{
	WRITE_LOCK_IDX(Map_LOCK);
	//cout << "StoreInFlightPacketFromSN : " << sn << endl;
	mSnToInFlightPacketMap.emplace(sn, inflightPacket);
}

InFlightPacketPtr DeliveryNotificationManager::EraseInFlightPacketFronSN(uint32 sn)
{
	
	WRITE_LOCK_IDX(Map_LOCK);
	//cout << "EraseInFlightPacketFronSN : " << sn << endl;
	auto iter = mSnToInFlightPacketMap.find(sn);
	if (iter == mSnToInFlightPacketMap.end())
		return nullptr;
	//ASSERT_CRASH(iter != mSnToInFlightPacketMap.end());

	InFlightPacketPtr inflightPacket = mSnToInFlightPacketMap[sn];
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
	if (!nextInFlightPacket)
		return nullptr;
	nextInFlightPacket->GetTransmissionData()->GotAck();
	return nextInFlightPacket;
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

void AckRange::AckWrite(OUT uint32& start, OUT int32& count, OUT bool& hasCount)
{
	start = mStart;
	hasCount = mCount > 1;
	if (hasCount)
	{
		int32 countMinusOne = mCount - 1; // 기준 값 제외 나머지 패킷 수
		uint8 countToAck = countMinusOne > 255 ? 255 : static_cast<uint8>(countMinusOne);
		count = static_cast<int32>(countToAck);
	}
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
