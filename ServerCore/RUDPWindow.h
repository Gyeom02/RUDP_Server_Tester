#pragma once

class RUDPRecvWindow
{

public:
	enum
	{
		WINDOW_SIZE = 1024,
	};

	explicit RUDPRecvWindow(int32 _expctedSeqNum);

	bool CheckRecved(int32 SeqNum);
	void DetachExpectedSeq();

	int32 GetExpectedSqeNum() { return _expctedSeqNum;  }
private:
	int32 _expctedSeqNum = 0;

	int32 windowSize = WINDOW_SIZE;

	vector<uint8> _RecvedBitMap;
};

class RUDPSendWindow
{
};