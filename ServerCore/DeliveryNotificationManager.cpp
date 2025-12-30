#include "pch.h"
#include "DeliveryNotificationManager.h"

DeliveryNotificationManager::DeliveryNotificationManager()
{
}

DeliveryNotificationManager::~DeliveryNotificationManager()
{
}

InFlightPacketPtr DeliveryNotificationManager::WriteSeqeuenceNumber(SOCKET object, NetAddress netAddr, SendBufferRef sendBuffer)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	PacketSequenceNumber sequenceNumber = mNextOutgoingSequenceNumber++;
	header->sn = sequenceNumber.GetSN(); // 패킷 헤더에 SequenceNumber 부착

	++mDispatchedPacketCount;

	WRITE_LOCK;

	mInFlightPackets.emplace_back(MakeShared<InFlightPacket>(object, netAddr, sequenceNumber, sendBuffer));
	return mInFlightPackets.back();
}

void DeliveryNotificationManager::ProcessAcks(int32 start, int32 count, bool hasCount)
{
	AckRange ackRange;
	ackRange.AckRead(start, count);

	PacketSequenceNumber nextAckdSequenceNumber = ackRange.GetStart();
	int32 onePastAckdSequenceNumber = nextAckdSequenceNumber.GetSN() + ackRange.GetCount();

	WRITE_LOCK;

	while (nextAckdSequenceNumber.GetSN() < onePastAckdSequenceNumber && !mInFlightPackets.empty())
	{
		const auto& nextInFlightPacket = mInFlightPackets.front();
		//패킷의 시퀀스 번호가 확인응답의 시퀀스 번호보다 작으면 확인응답을 받지 못한 것이므로 아마 누락되었을 것임
		PacketSequenceNumber nextInFlightPacketSequenceNumber = nextInFlightPacket->GetSequenceNumber();

		if (nextInFlightPacketSequenceNumber.GetSN() < nextAckdSequenceNumber.GetSN())
		{
			//사본을 만든다음, 목록에서 일단 제거함
			//핸들링 도중 살아있는 패킷이 무엇인지 찾아볼 때 이 패킷에 보여서는 안되기 때문이다.
			auto copyOfInFlightPacket = nextInFlightPacket;
			mInFlightPackets.pop_front();
			HandlePacketDeliveryFailure(copyOfInFlightPacket);
			mSequenceNotMatchedCount++;
			//cout << nextInFlightPacketSequenceNumber.GetSN() << " < " << nextAckdSequenceNumber.GetSN() << endl;
		}
		else if (nextInFlightPacketSequenceNumber == nextAckdSequenceNumber)
		{
		//	cout << "DeliverySuccess" << endl;
			HandlePacketDeliverySuccess(nextInFlightPacket);
			mInFlightPackets.pop_front();
			++nextAckdSequenceNumber;
		}

		else if (nextInFlightPacketSequenceNumber > nextAckdSequenceNumber)
		{
			//일부 응답이 어떤 연유에선지 제거되었음(시간 초과 가능성)
			//나머지를 계속하여 검사함
		//	cout << "nextInFlightPacketSequenceNumber > nextAckdSequenceNumber" << endl;
			nextAckdSequenceNumber = nextInFlightPacketSequenceNumber;
		}
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

bool DeliveryNotificationManager::ProcessSequenceNumber(PacketSequenceNumber SN)
{
	if (SN.GetSN() >= mNextExpectedSequenceNumber) //예상하던 수신 패킷 세퀀스넘버가 맞음
	{
		mNextExpectedSequenceNumber = SN.GetSN() + 1;
		AddPendingAck(SN);
		return true;
	}

	else if (SN.GetSN() < mNextExpectedSequenceNumber) //기다리고 있었던 수신 패킷 세퀀스 넘버가 아님 조용히 넘김
	{
		return false;
	}

	return false;

}

void DeliveryNotificationManager::AddPendingAck(PacketSequenceNumber SN)
{
	WRITE_LOCK;
	if (mPendingAcks.size() == 0 || !mPendingAcks.back().ExtendIfShould(SN))
	{
		mPendingAcks.emplace_back(SN.GetSN());
	}
}

bool DeliveryNotificationManager::WritePendingAcks(OUT int32& start, OUT int32& count, OUT bool& hasCount)
{
	WRITE_LOCK;
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
	WRITE_LOCK;
	uint64 now = GetTickCount64();
	while (!mInFlightPackets.empty())
	{
		const auto& nextInFlightPacket = mInFlightPackets.front();
		if (now - nextInFlightPacket->GetTimeDispactched() > TIMEOUT)
		{
			mTimeOutCount++;
			PacketHeader* header = reinterpret_cast<PacketHeader*>(nextInFlightPacket->GetTransmissionData()->Buffer());
		//	cout << " TimeOutPacket ID : " << header->id << " | SN : " << header->sn << endl;
			HandlePacketDeliveryFailure(nextInFlightPacket);
			mInFlightPackets.pop_front();
		}
		else
			return; //이후 다음 패킷부터는 초과가 아님(시간순서대로 넣어져있기 때문이다)
	}
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

void AckRange::AckWrite(OUT int32& start, OUT int32& count, OUT bool& hasCount)
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

void AckRange::AckRead(int32 start, int32 count)
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
