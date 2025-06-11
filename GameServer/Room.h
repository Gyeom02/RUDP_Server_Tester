#pragma once
#include "JobQueue.h"
constexpr size_t SHARD_COUNT = 64;

class Room : public JobQueue
{
public:
	enum : int32
	{
		READYROOM = 1,
		SELECTROOM = 2,
		INGAME = 3,
		VICTORY = 4,
		ROOMMAX = 8
	};
	// 싱글쓰레드 환경인마냥 코딩
	Room(int32 roomid);
	virtual ~Room() { cout << "Room " << roomId << "Destructor" << endl; }

	void Enter(PlayerRef player);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);

	int32 GetRoomID() { return roomId; }
	void SetRoomID(int32 newid) { roomId = newid; }

	map<int32, PlayerRef>& GetPlayers() { return _players; }
	PlayerRef GetPlayer(int32 id); 
	int32 PlayerNum() { return _players.size(); }

	int32 GivePrimID();
	void TakePrimID(int32 id);

	int32 FirstGiveListID(int32 prim); //처음 들어온 플레이어에게 ListID를 지정해주는 함수
	int32 GiveListID(int32 prim); //front value를 내보냄
	int32 GiveSelectListID(int32 index); // 원하는 값을 추출
	void TakeListID(int32 id); //플레이어가 방에서 나가거나 Team을 바꿀때 사용되는 함수


	int32 GetListToPrim(int32 list) { return listToPrim[list]; }
	int32 GetPrimToList(int32 prim) { return primToList[prim]; }

	bool SetListID(int32 prelist, int32 list, int32 prim);
	void EraseListToPrim(int32 list);
	void ErasePrimToList(int32 prim);

	void ListSort(int32 teamNum, int32 index); //TeamMode와 TeamNum에 따라서 TeamNum에 해당된 List만 정렬한다
	int32 GetTeamNum(int32 index);

	void SetTeamMode(int32 mode) { TeamModeNum = mode; }
	int32 GetTeamMode() { return TeamModeNum; }

	void SetRoomFlow(int32 flow) { _roomFlow = flow; }
	int32 GetRoomFlow() { return _roomFlow; }

	Vector<Protocol::MoveList> ResetTeamNum(); //방에 있는 모든 플레이어들의 TeamNum을 재정립시켜주는 함수

	void ListSortFromZero(); // List를 1부터 차례대로 정렬한다

	void AddReady() { _readys++; }
	void MinusReady() { _readys--; }
	void SetReady(int32 b) { _readys.store(b); }
	void ResetReady() { SetReady(0); }
	int32 GetReady() { return _readys.load(); }

private:
	map<int32, PlayerRef> _players; // roomId, PlayerRef
	List<int32> _ListReuseListID; // ListID의 재활용되는 ID 리스트 Upper
	map<int32, int32> listToPrim; // List, Prim  ReadyRoom Level에서 플레이어들의 UI 동기화에 쓰일 자료
	map<int32, int32> primToList; // List, Prim  ReadyRoom Level에서 플레이어들의 UI 동기화에 쓰일 자료
	Queue<int32> _ListReusePrimID;
	atomic<int32> roomId = -1;
	atomic<int32> TeamModeNum = 1; // 1 = 8팀, 2 = 4팀, 3 = 2팀
	atomic<int32> _roomFlow = READYROOM;
	atomic<int32> _readys = 0;
	USE_LOCK;
};

using RoomRef = shared_ptr<Room>;

class RoomShard
{
public:
	void Make(RoomRef room);
	void Remove(RoomRef room);
	void Broadcast(SendBufferRef sendBuffer);
	RoomRef GetRoom(int32 id);
	unordered_map<int, RoomRef> _rooms;
	USE_LOCK;
};

class Rooms
{
public:
	// 싱글쓰레드 환경인마냥 코딩
	void Make(RoomRef room);
	void Remove(RoomRef room);
	void Broadcast(SendBufferRef sendBuffer);
	RoomRef GetRoom(int32 id); 

private:
	Array<RoomShard, SHARD_COUNT> _roomShards;
	//USE_LOCK;
};

extern shared_ptr<Rooms> GRoom;

