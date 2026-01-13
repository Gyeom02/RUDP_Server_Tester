#pragma once
#include "RUDPWindow.h"

class FragmentContext
{
public:

	FragmentContext(PacketHeader* header, uint32 primid, int16 frag_count, int16 original_size);

	void Store(BYTE* buffer, int32 size, int16 offset, int16 index);
	std::vector<BYTE> _storedBuffer;
	
	
	bool IsAllStored() { return (stored_frag_count == frag_Count); }
	int16 GetSize() { return stored_size; }
	uint32 GetNextExpectedSeqNum() { return primID + 1; }


	void SetPrimID(uint32 id) { primID = id; }
private:

	vector<uint8> _recievedBitMap;

	atomic<int16> stored_frag_count = 0; // 현재 얻은 조각화 패킷 수
	atomic<int16> stored_size = 0; // 현재 저장된 사이즈;

	int16 frag_Count = -1; // 총 조각화된 패킷의 수
	int16 original_Size = -1; // 원본 패킷의 PayLoad 사이즈(Byte 단위)

	uint32 primID = -1; // SN (조각화 패킷한테는 제일 먼저 보내진 SN가 가장 작은 것을 primID로 삼는다)
	
/*------Fragment 패킷중 제일 작은 SN 찾기 위해 사용되는 함수, 변수들--------*/
public:
	uint32 GetSmallestSNNum() { return  fragmentSmallestSNNum; }
	void SetSmallestSNNum(uint32 sn)
	{
		if (fragmentSmallestSNNum == 0) //한번도 접근하지 않은 상태
		{
			fragmentSmallestSNNum = sn;
		}
		fragmentSmallestSNNum = fragmentSmallestSNNum > sn ? sn : fragmentSmallestSNNum;
	}
	priority_queue<uint32, vector<uint32>, greater<uint32>> _fragmentSeqNums; //fragcfxs에서는 조각화 패킷들의 SN들의 정렬모음, 
private:
	uint32 fragmentSmallestSNNum = 0;
	
/*---------------------*/ 
};

class Ordered_FG_Manager // RO 패킷의 Ordered와 패킷 조각화를 관리하는 클래스
{
private:
	enum
	{
		//MAX = RUDPWIND::SN_MAX_SIZE,
	};
public: //API
	bool OnOrderedRecv(uint32 seqNum, BYTE* buffer, int32 size, OUT queue<vector<BYTE>>& outReadyQueue);

	//bool OnRecv(BYTE* buffer, int32 size, OUT vector<BYTE>& outBuffer, OUT int32& outsize);
	bool OnFragment(uint32 seqNum, BYTE* buffer, int32 size, OUT queue<vector<BYTE>>& outReadyQueue);


	//bool PopReadyPacket(OUT std::vector<BYTE>& _outv);
	//void PushReadyPacket(const std::vector<BYTE>& _packet);

	bool IsThereReadyPacket(uint32 expectedseqNum, OUT queue < vector<BYTE>>& _queue);

public: //
	Ordered_FG_Manager() :_fragmentPassSNs(RUDPWIND::SN_MAX_SIZE, false) {}

protected:
	bool CheckCanSNPass(uint32 sn) { if (_fragmentPassSNs[sn % RUDPWIND::SN_MAX_SIZE]) return true; else return false; }
	bool TryPassSN(uint32 sn) { if (CheckCanSNPass(sn)) { _fragmentPassSNs[sn % RUDPWIND::SN_MAX_SIZE] = false; return true; } else  return false; }
	void PushPassSN(uint32 sn) { _fragmentPassSNs[sn % RUDPWIND::SN_MAX_SIZE] = true; }
private:
	std::map<uint32, shared_ptr<FragmentContext>> _orderedCtxs; // < sn,  shared_ptr<FragmentContext>> 을 pair로 갖는 map 
	std::map<uint32, shared_ptr<FragmentContext>> _fragCtxs; // < Fragment::PrimID,  shared_ptr<FragmentContext>> 을 pair로 갖는 map / 조각화가 모두 모이면 _orderdCtxs로 옮겨짐
	queue<std::vector<BYTE>> _readyPacket;

	uint32 _expectedSeqNum = 0;
	vector<uint8> _fragmentPassSNs;

	//priority_queue<int16, vector<int16>, greater<int16>> _fragmentPassSNs; //orderdcfs에서는 조각화 패킷들의 패스 키 정렬 배열
	USE_LOCK;
};

