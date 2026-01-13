#pragma once

namespace RUDPWIND
{
	enum
	{
		SN_MAX_SIZE = 50,
		SN_RANGE_HALF = SN_MAX_SIZE / 2,
		RWIND_BASE_SIZE = 64000, // 64kb라는뜻 보통 FPS Server은 64~128, MMO Server은 128~256을 가진다
	};
}

class RUDPRecvWindow // Recv할때 송신측이 보낸 패킷의 sn을 확인하고 ordered하게 처리 할 수 있도록 컨트롤 함, 추가적으로 개인의 rwind를 보관하고 있음 ack을 보낼때 해당 정보를 보냄
{

public:
	
	explicit RUDPRecvWindow(uint32 _expctedSeqNum);


	bool CheckRecved(uint32 SeqNum);
	void DetachExpectedSeq();

	uint32 GetExpectedSqeNum() { return _expctedSeqNum;  }
private:
	uint32 _expctedSeqNum = 0;

	//uint32 windowSize = RUDPWIND::SN_RANGE_HALF;

	vector<uint8> _RecvedBitMap;



public: 	/* rWind */
	bool IsSpaceExistToRecv(int32 recvSize) {
		if (recvSize <= 0) return false;
		if (_myRWind.load() >= recvSize) { _myRWind.fetch_sub(recvSize); return true; }
		else return false;
	}
	int32 GetRWind() { return _myRWind.load(); }
	void MakeSpaceRWind(int32 doneRecvSize) { if (doneRecvSize <= 0) return; _myRWind.fetch_add(doneRecvSize); }
private:	/* rWind */
	const int32 _RWind_Fixed_Max;
	const int32 _RWind_Fixed_Lowest;
	atomic<int32> _myRWind;
};

class RUDPSendWindow // Send 할때 수신측의 rwind, cwind를 확인하고 보낼지 말지를 컨트롤함
{
public:

	explicit RUDPSendWindow(int32 windSize = RUDPWIND::RWIND_BASE_SIZE) : _receiverRWind(windSize), _myCWind(windSize){}

	bool IsSpaceExistToSend(int32 sendSize) {
		if (sendSize <= 0) return false;
		if (_receiverRWind.load() >= sendSize) { _receiverRWind.fetch_sub(sendSize); return true; }
		else return false;
	}

	void AddReceiverRWind(int32 size) { _receiverRWind.fetch_add(size); }
	int32 GetReceiverRWind() { return _receiverRWind.load(); }
private:
	atomic<int32> _receiverRWind;
	atomic<int32> _myCWind;

	
};