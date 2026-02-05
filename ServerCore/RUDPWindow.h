#pragma once

namespace RUDPWIND
{
	enum
	{
		OVERHEAD = 100,
		SN_MAX_SIZE = 65536, // index 최대값 + 1 의 값
	 	SN_RANGE_HALF = SN_MAX_SIZE / 2,
		//RWIND_MAX_SIZE = 64000,
		/*RWIND_BASE_SIZE = RWIND_MAX_SIZE - DEFAULT_MTU_SIZE, */
		RWIND_BASE_SIZE = 64 * 1024, // 64kb라는뜻 보통 FPS Server은 64~128, MMO Server은 128~256을 가진다
		FRAG_BITMAP_INDEX_MAX = 65536,
		SEND_BUFFER_CHUNK_SIZE = 32000 + OVERHEAD, // Header와 Payload 전체 합쳐서 패킷 최대 크기
		
		OFO_MAX_SIZE = RWIND_BASE_SIZE - SEND_BUFFER_CHUNK_SIZE - OVERHEAD,
	};
}

class RUDPRecvWindow // Recv할때 송신측이 보낸 패킷의 sn을 확인하고 ordered하게 처리 할 수 있도록 컨트롤 함, 추가적으로 개인의 rwind를 보관하고 있음 ack을 보낼때 해당 정보를 보냄
{

public:
	
	explicit RUDPRecvWindow(uint32 _expctedSeqNum = 0);


	bool CheckRecved(uint32 SeqNum);
	bool CheckSize(uint32 SeqNum, int32 size);

	bool CheckAndTryFrag(struct PacketHeader* header,  struct FragmentHeader* fragHeader, int32 needSpaceSize);
	
	void TryRecv(uint32 SeqNum); // Only When CheckRecved return True
	//void TryFrag(FragmentHeader* fragHeader, uint32 sn, int32 size);
	void DetachExpectedSeq();
	//void DetechLastFrag();
	uint32 GetExpectedSqeNum() { return _expctedSeqNum.load();  }
private:
	atomic<uint32> _expctedSeqNum = 0; // it's not atomic or need Lock, fundemental of this is one socket(recvfrom)
	uint32 _ofo_Valid_Wind = 0;
	//uint32 windowSize = RUDPWIND::SN_RANGE_HALF;

	vector<uint8> _RecvedBitMap; // For Out Of Order Packets
	//vector<uint8> _FragBitMap;

	unordered_map<uint32, int16> _frag_storeCountMap;

public: 	/* rWind */
	bool IsSpaceExistToRecv(int32 recvSize) {
		if (recvSize <= 0) return false;
		if (_myRWind.load() >= recvSize) { return true; }
		else return false;
	}
	int32 GetRWind() { return _myRWind.load(); }
	void MakeSpaceRWind(int32 doneRecvSize) { if (doneRecvSize <= 0) return; _myRWind.fetch_add(doneRecvSize); }
	bool ReduceSpaceRWind(int32 size) { if (size <= 0) return false; _myRWind.fetch_sub(size); return true; }

	uint32 GetTotalRWind() { return _totalRecoverRWind.load(); }
	void AddTotalRWind(uint32 doneRecvSize) { if (doneRecvSize <= 0) return; _totalRecoverRWind.fetch_add(doneRecvSize); }
	//void SubTotalRWind(int32 size) { if (size <= 0) return; _totalRecoverRWind.fetch_sub(size); }
private:	/* rWind */
	const int32 _RWind_Fixed_Max;
	const int32 _RWind_Fixed_Lowest;
	atomic<int32> _myRWind;
	atomic<uint32> _totalRecoverRWind;

};

class RUDPSendWindow // Send 할때 수신측의 rwind, cwind를 확인하고 보낼지 말지를 컨트롤함
{
public:

	explicit RUDPSendWindow(int32 windSize = RUDPWIND::RWIND_BASE_SIZE) : _receiverRWind(windSize), _totalRecoverRWind(0), _myCWind(windSize){}

	bool IsSpaceExistToSend(int32 sendSize) {
		if (sendSize <= 0) return false;
		if (_receiverRWind.load() >= sendSize) { _receiverRWind.fetch_sub(sendSize); return true; }
		else return false;
	}

	void AddReceiverRWind(int32 size) { _receiverRWind.fetch_add(size); }
	int32 GetReceiverRWind() { return _receiverRWind.load(); }

	uint32 GetTotalRWind() { return _totalRecoverRWind.load(); }
	void SetTotalRWind(uint32 doneRecvSize) { if (doneRecvSize <= 0) return; _totalRecoverRWind.exchange(doneRecvSize); }

	void StoreReceiverRWind(int32 rwindsize) { _receiverRWind.store(rwindsize); }
private:
	atomic<int32> _receiverRWind;
	atomic<uint32> _totalRecoverRWind;
	atomic<int32> _myCWind;
	//USE_LOCK;
	
};