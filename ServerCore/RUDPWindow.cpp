#include "pch.h"
#include "RUDPWindow.h"

RUDPRecvWindow::RUDPRecvWindow(uint32 _expctedSeqNum)
	: _expctedSeqNum(_expctedSeqNum), _RecvedBitMap(RUDPWIND::SN_MAX_SIZE, 0), _RWind_Fixed_Max(RUDPWIND::RWIND_BASE_SIZE), _RWind_Fixed_Lowest(0), _myRWind(_RWind_Fixed_Max)
{
	
#ifdef _DEBUG
	ASSERT_CRASH(_myRWind.is_lock_free());
#endif
}


bool RUDPRecvWindow::CheckRecved(uint32 SeqNum)
{
	//int32 index = (SeqNum - _expctedSeqNum) % WINDOW_SIZE;
	int32 index = SeqNum % RUDPWIND::SN_MAX_SIZE;
	if (index < 0)
		return false;

	if (_RecvedBitMap[index] == 1)
		return false;

	if (_expctedSeqNum == SeqNum) //기다리던 패킷이 왔다.
		DetachExpectedSeq();

	_RecvedBitMap[index] = 1;
	//cout << "RUDPRecvWindow::CheckRecved true" << endl;
	return true;
}

void RUDPRecvWindow::DetachExpectedSeq() // 예상하던 Seq의 패킷이 들어옴
{
	
	_expctedSeqNum++;
	_RecvedBitMap[_expctedSeqNum % RUDPWIND::SN_MAX_SIZE] = 0;
	_RecvedBitMap[RUDPWIND::SN_RANGE_HALF - 1] = 0;
}
