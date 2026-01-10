#pragma once

namespace RUDPWIND
{
	enum
	{
		SN_MAX_SIZE = 0xFFFF + 1,
		SN_RANGE_HALF = SN_MAX_SIZE / 2,
	};
}

class RUDPRecvWindow
{

public:
	
	explicit RUDPRecvWindow(int32 _expctedSeqNum);

	bool CheckRecved(int32 SeqNum);
	void DetachExpectedSeq();

	int32 GetExpectedSqeNum() { return _expctedSeqNum;  }
private:
	int32 _expctedSeqNum = 0;

	int32 windowSize = RUDPWIND::SN_RANGE_HALF;

	vector<uint8> _RecvedBitMap;
};

class RUDPSendWindow
{
};