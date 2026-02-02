#include "pch.h"
#include "RUDPWindow.h"

RUDPRecvWindow::RUDPRecvWindow(uint32 _expctedSeqNum)
	: _expctedSeqNum(_expctedSeqNum), _ofo_Valid_Wind(RUDPWIND::OFO_MAX_SIZE), _RecvedBitMap(RUDPWIND::SN_MAX_SIZE, 0),  _RWind_Fixed_Max(RUDPWIND::RWIND_BASE_SIZE), _RWind_Fixed_Lowest(0), _myRWind(_RWind_Fixed_Max), _totalRecoverRWind(0)
{
	
#ifdef _DEBUG
	ASSERT_CRASH(_myRWind.is_lock_free());
#endif
}


bool RUDPRecvWindow::CheckRecved(uint32 SeqNum)
{
	//int32 index = (SeqNum - _expctedSeqNum) % WINDOW_SIZE;

	if (SeqNum < 0)
		return false;
	if (SeqNum >= _expctedSeqNum.load() + RUDPWIND::SN_RANGE_HALF)
		return false;

	int32 index = SeqNum % RUDPWIND::SN_MAX_SIZE;
	
	if (_RecvedBitMap[index] != 0)
		return false;
	
	//cout << "RUDPRecvWindow::CheckRecved true" << endl;
	return true;
}

bool RUDPRecvWindow::CheckSize(uint32 SeqNum, int32 size) // Only For Single Packet Not Fragment Packets
{
#ifdef _DEBUG
	ASSERT_CRASH(size > 0);
#endif

	if (SeqNum == _expctedSeqNum.load())
		return true;
	if (_ofo_Valid_Wind < size)
		return false;
	return true;
}

bool RUDPRecvWindow::CheckAndTryFrag(PacketHeader* header, FragmentHeader* fragHeader, int32 needSizeSpace) // needSizeSpace is frag all size
{
#ifdef _DEBUG
	ASSERT_CRASH(needSizeSpace > 0);
#endif
	//Fragment 패킷임
	//FragmentHeader* fragHeader = reinterpret_cast<FragmentHeader*>(&header[1]);
	//uint16 fragPrimID = fragHeader->primID;
	const int32 dummySize = -1; //fragHeader->first_sn == _expctedSeqNum.load() = -1, else = packetSize
	
	auto iter = _frag_storeCountMap.find(fragHeader->first_sn);

	if (fragHeader->first_sn == _expctedSeqNum.load()) // Fragment 패킷의 제일 앞 패킷의 SN이 예상하던 SN 값이다
	{
		int32 index = header->sn % RUDPWIND::SN_MAX_SIZE;
		//dummySize = -1;
		_RecvedBitMap[index] = dummySize;

		if (iter != _frag_storeCountMap.end()) //현재 찾고있는 Frag 패킷이다
		{
			iter->second += 1;
			if (iter->second == fragHeader->frag_count) // Fragment 전부 모임
			{
				_frag_storeCountMap.erase(fragHeader->first_sn);
				//TODO 
				DetachExpectedSeq();
			}
			
		}
		else // 처음보는 Frag 패킷이다
		{
			/*if (!IsSpaceExistToRecv(needSizeSpace))
			{
				cout << "1 Check Frag Out Of RWind : " << GetRWind() << " < " << needSizeSpace << endl;

				return false;
			}
			ReduceSpaceRWind(needSizeSpace);*/

			_frag_storeCountMap.insert(make_pair(fragHeader->first_sn, 1));
		}
		
	}
	else // 기다리는 SN을 FirstSn으로 갖고있지 않은 Out Of Order Fragment 패킷 중 하나이다
	{
		
		if (iter != _frag_storeCountMap.end()) //현재 찾고있는 Frag 패킷이다
		{
			if (header->sn != fragHeader->first_sn)
			{
				int32 index = header->sn % RUDPWIND::SN_MAX_SIZE;
				_RecvedBitMap[index] = dummySize;

			}
			iter->second += 1;
			if (iter->second == fragHeader->frag_count) // Fragment 전부 모임
			{
				_frag_storeCountMap.erase(fragHeader->first_sn);
			}

			//dummySize = header->size;
		}
		else // 처음보는 Frag PrimID이다
		{
			/*if (!IsSpaceExistToRecv(needSizeSpace))
			{
				cout << "2 Check Frag Out Of RWind : " << GetRWind() << " < " << needSizeSpace << endl;

				return false;
			}

			
			ReduceSpaceRWind(needSizeSpace);*/
			int32 index = fragHeader->first_sn % RUDPWIND::SN_MAX_SIZE;

			if (_ofo_Valid_Wind < header->size) // ? header->size or  needSizeSpace
			{
				cout << "CheckFrag _ofo_Valid_Wind < header->size" << endl;
				return false;
			}
			_ofo_Valid_Wind -= needSizeSpace;
			_RecvedBitMap[index] = needSizeSpace;

			_frag_storeCountMap.insert(make_pair(fragHeader->first_sn, 1));
			//dummySize = header->size;
		}
		//ummySize = header->size;
		
	}
		/*if (!IsSpaceExistToRecv(needSizeSpace))
		{
			cout << "CheckFrag Out Of RWind | " << needSizeSpace << " > " << _myRWind << endl;

			return false;
		}*/
		
		//_FragBitMap[fragPrimID] = 1; // 설정
	
	
	
	return true;
}

void RUDPRecvWindow::TryRecv(uint32 SeqNum)
{
//#ifdef _DEBUG
//	ASSERT_CRASH(size > 0);
//#endif
	int32 index = SeqNum % RUDPWIND::SN_MAX_SIZE;
	if (_expctedSeqNum.load() == SeqNum) //기다리던 패킷이 왔다.
		DetachExpectedSeq();
	else
	{
		_RecvedBitMap[index] = 1;
		//_ofo_Valid_Wind -= size;
	}
}

//void RUDPRecvWindow::TryFrag(FragmentHeader* fragHeader, uint32 sn, int32 size)
//{
//#ifdef _DEBUG
//	ASSERT_CRASH(size > 0);
//#endif
//
//	int32 index = SeqNum % RUDPWIND::SN_MAX_SIZE;
//	if (_expctedSeqNum.load() == SeqNum) //기다리던 패킷이 왔다.
//	{
//		DetachExpectedSeq();
//	}
//
//	else
//	{
//		_RecvedSizeMap[index] = size;
//		_ofo_Valid_Wind -= size;
//	}
//}

void RUDPRecvWindow::DetachExpectedSeq() // 예상하던 Seq의 패킷이 들어옴
{

	
	while (true)
	{
		int32 expectSNIndex = _expctedSeqNum % RUDPWIND::SN_MAX_SIZE;
		_RecvedBitMap[(expectSNIndex + RUDPWIND::SN_RANGE_HALF) % RUDPWIND::SN_MAX_SIZE] = 0;
		_RecvedBitMap[expectSNIndex] = 0;
		_expctedSeqNum++;

		if (_RecvedBitMap[_expctedSeqNum.load() % RUDPWIND::SN_MAX_SIZE] == 0)
				break;
	}
	//while (true)
	//{
	//	uint32 expectedSN = _expctedSeqNum.load();
	//	if (_frag_storeCountMap.find(expectedSN) != _frag_storeCountMap.end()) //해당 SN을 FirstSN으로 하는 Fragment를 모집중 아직 다 안 모음
	//		return;
	//	int32 expectSNIndex = expectedSN % RUDPWIND::SN_MAX_SIZE;

	//	if (_RecvedSizeMap[expectSNIndex] > 0) //Out Of Ordered Fragment Packet 또는 Single Packet
	//	{
	//		_ofo_Valid_Wind += _RecvedSizeMap[expectSNIndex];
	//	}
	//	_RecvedSizeMap[(expectSNIndex + RUDPWIND::SN_RANGE_HALF) % RUDPWIND::SN_MAX_SIZE] = 0;
	//	_RecvedSizeMap[expectSNIndex] = 0;
	//	_expctedSeqNum.fetch_add(1);

	//	if (_RecvedSizeMap[_expctedSeqNum.load() % RUDPWIND::SN_MAX_SIZE] == 0)
	//		break;

	//	
	//}
}
//
//void RUDPRecvWindow::DetechLastFrag()
//{
//	while (true)
//	{
//		int32 expectSNIndex = _expctedSeqNum.load() % RUDPWIND::SN_MAX_SIZE;
//		_RecvedSizeMap[(expectSNIndex + RUDPWIND::SN_RANGE_HALF) % RUDPWIND::SN_MAX_SIZE] = 0;
//	}
//}
