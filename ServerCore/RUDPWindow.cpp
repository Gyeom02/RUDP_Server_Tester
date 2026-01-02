#include "pch.h"
#include "RUDPWindow.h"

RUDPRecvWindow::RUDPRecvWindow(int32 _expctedSeqNum)
 : _expctedSeqNum(_expctedSeqNum), _RecvedBitMap(WINDOW_SIZE, 0)
{
	
}

bool RUDPRecvWindow::CheckRecved(int32 SeqNum)
{
	//int32 index = (SeqNum - _expctedSeqNum) % WINDOW_SIZE;
	int32 index = SeqNum % WINDOW_SIZE;
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
	_RecvedBitMap[_expctedSeqNum % WINDOW_SIZE] = 0;
	_RecvedBitMap[WINDOW_SIZE - 1] = 0;
}
