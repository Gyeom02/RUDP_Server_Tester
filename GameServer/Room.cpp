#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"

shared_ptr<Rooms> GRoom = make_shared<Rooms>();

Room::Room(int32 roomid)
: roomId(roomid)
{
	for (int32 i = 1; i <= 8; i++)
		_ListReusePrimID.push(i);
	for (int32 i = 1; i <= 8; i++)
		_ListReuseListID.push_back(i);
}

void Room::Enter(PlayerRef player)
{
	//_players.insert(make_pair(player->playerId, player));
	WRITE_LOCK;
	_players[player->roomprimid] = player;
}

void Room::Leave(PlayerRef player)
{
	WRITE_LOCK;
	//player 정보 초기화
	player->roomId = -1;
	

	//room prim, list등 처리
	SetListID(primToList[player->roomprimid], -1, player->roomprimid);
	TakePrimID(player->roomprimid);

	_players.erase(player->roomprimid);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto& p : _players)
	{
	//	cout << p.second->playerId << endl;
		p.second->Send(sendBuffer);
	}
}

PlayerRef Room::GetPlayer(int32 id)
{
	READ_LOCK;
	auto player = _players.find(id);
	if (player == _players.end()) // 존재하지않음
		return nullptr;

	return (player->second);
}

int32 Room::GivePrimID()
{
	WRITE_LOCK;
	if (_ListReusePrimID.empty())
		return -1;

	int32 id = _ListReusePrimID.front();
	_ListReusePrimID.pop();

	return id;
}

void Room::TakePrimID(int32 id)
{
	WRITE_LOCK;
	_ListReusePrimID.push(id);
}

int32 Room::FirstGiveListID(int32 prim)
{
	int32 id = GiveListID(prim);
	SetListID(-1, id, prim);
	return id;
}

int32 Room::GiveListID(int32 prim)
{
	WRITE_LOCK;
	if (_ListReuseListID.empty())
		return -1;

	int32 id = _ListReuseListID.front();
	_ListReuseListID.pop_front();

	return id;
}

int32 Room::GiveSelectListID(int32 index) // Lock 필요없음
{

	auto iter = std::find(_ListReuseListID.begin(), _ListReuseListID.end(), index);
	if (iter == _ListReuseListID.end())
		return -1;
	_ListReuseListID.erase(iter);
	return index;
}

void Room::TakeListID(int32 id) // LOCK 필요없음
{
	_ListReuseListID.push_back(id);
	_ListReuseListID.sort();
}

bool Room::SetListID(int32 prelist, int32 list, int32 prim) // 0, 0, 0 -> 단순 List 변경, 0, -1, 0 -> 방을 나가는 게 목적, -1, 0, 0 -> 새로 들어오는 플레이어
{
	WRITE_LOCK
	if (listToPrim.find(list) != listToPrim.end()) //이미 다른 플레이어가 갖고있는 ListID임 
		return false;
	if (prelist != -1) //처음 들어오는 플레이어가 아님
	{
		TakeListID(prelist);
		GiveSelectListID(list);
		EraseListToPrim(prelist);
		ErasePrimToList(prim);
		cout << "prelist != -1" << endl;
	}
	else
		cout << "prelist == -1" << endl;
	if (list != -1) // 방을 나가는게 아닌 단순 List 변경이다
	{
		listToPrim[list] = prim;
		primToList[prim] = list;
	}
	return true;
}

void Room::EraseListToPrim(int32 list) // Lock 필요없음
{
	listToPrim.erase(list);
}

void Room::ErasePrimToList(int32 prim)// Lock 필요없음
{
	primToList.erase(prim);
}

void Room::ListSort(int32 teamNum, int32 index) //좀 더 LOCK 수정 봐야함
{
	WRITE_LOCK;
	if (TeamModeNum == 1) //8팀 //
	{
		int32 teammax = 8; //1팀이면 4 2팀이면 8임
		int32 less = (teammax - index); // ex  teammax = 4, index = 2 , -> less = 2
		int32 startindex = index + 1;
		Protocol::S_MOVETEAM pkt;

		while (less > 0)
		{
			if (listToPrim.find(startindex) == listToPrim.end()) //
				break;
			PlayerRef p = GetPlayer(listToPrim[startindex]);
			int32 nextindex = startindex - 1; // startindex가 이동하려는 listid

			Protocol::MoveList* inform = pkt.add_moveinforms();
			inform->set_roomprimeid(listToPrim[startindex]);
			inform->set_roomlistid(startindex);
			inform->set_movetolistid(nextindex);

			p->teamNum = GetTeamNum(nextindex);
			inform->set_moveteamnum(p->teamNum);

			SetListID(startindex, nextindex, listToPrim[startindex]);


			startindex++;
			less--;
		}
		SendBufferRef sendBuffer = ClientPacketHandler::MakeReliableBuffer(pkt, QoSCore::HIGH);
		Broadcast(sendBuffer);

		

	}

    else if (TeamModeNum == 2) //4팀
	{
		int32 startindex = (teamNum * 2) - 1;
		if (startindex != index)
			return;
		int32 sortindex = startindex + 1;

		if (listToPrim.find(sortindex) == listToPrim.end()) //
			return;

		Protocol::S_MOVETEAM pkt;
		Protocol::MoveList* inform = pkt.add_moveinforms();
		inform->set_roomprimeid(listToPrim[sortindex]);
		inform->set_roomlistid(sortindex);
		inform->set_movetolistid(startindex);
		inform->set_moveteamnum(-1);

		SetListID(sortindex, startindex, listToPrim[sortindex]);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeReliableBuffer(pkt, QoSCore::HIGH);
		Broadcast(sendBuffer);
	}
	else if (TeamModeNum == 3) //2팀
	{
		int32 teammax = ((4 - index) >= 0) ? 4 : 8; //1팀이면 4 2팀이면 8임
		int32 less = (teammax - index) ; // ex  teammax = 4, index = 2 , -> less = 2
		int32 startindex = index + 1;
		Protocol::S_MOVETEAM pkt;
		while (less > 0)
		{
			if (listToPrim.find(startindex) == listToPrim.end()) //
				break;
			int32 nextindex = startindex - 1; // startindex가 이동하려는 listid
			

			Protocol::MoveList* inform = pkt.add_moveinforms();
			inform->set_roomprimeid(listToPrim[startindex]);
			inform->set_roomlistid(startindex);
			inform->set_movetolistid(nextindex);
			inform->set_moveteamnum(-1);

			SetListID(startindex, nextindex, listToPrim[startindex]);

			startindex++;
			less--;
		}
		SendBufferRef sendBuffer = ClientPacketHandler::MakeReliableBuffer(pkt, QoSCore::HIGH);
		Broadcast(sendBuffer);
	}
}

int32 Room::GetTeamNum(int32 index)
{
	READ_LOCK;
	if (TeamModeNum == 1) //8팀
	{
		return index;
	}

	else if (TeamModeNum == 2) //4팀
	{
		if (index <= 2) // 첫번째팀
			return 1;
		else if (index <= 4)
			return 2;
		else if (index <= 6)
			return 3;
		else
			return 4;
	}

	else if (TeamModeNum == 3) //2팀
	{
		if (index <= 4)
			return 1;
		else return 2;
	}

	
}

Vector<Protocol::MoveList> Room::ResetTeamNum() //List Sort를 끝낸후 쓰는 함수
{
	Vector<Protocol::MoveList> informV;
	{
		READ_LOCK;
		int32 listsize = ROOMMAX - _ListReuseListID.size(); //방안에 있는 플레이어 수
	}//쓸수 있는 최저 ListID찾기
	
	
	ListSortFromZero();

	READ_LOCK;
	for (auto p : _players)
	{
		Protocol::MoveList playerP;
		PlayerRef player = p.second;

		//ASSERT_CRASH(player != nullptr);

		int32 primid = player->roomprimid;

		player->teamNum = GetTeamNum(primToList[primid]);

		playerP.set_roomprimeid(primid);
		playerP.set_roomlistid(-1);
		playerP.set_moveteamnum(player->teamNum);
		playerP.set_movetolistid(primToList[primid]);
		informV.push_back(playerP);
		cout << "movetolistid : " << informV.back().movetolistid() << "MoveTeamNum : " << informV.back().moveteamnum() << "roomprimeid : " << informV.back().roomprimeid() << endl;
	}
	/*
	* Vector<Protocol::MoveTeam> informV;
	int32 listsize = ROOMMAX - _ListReuseListID.size(); //방안에 있는 플레이어 수

	for (int32 start = 1; start <= listsize; start++ )
	{
		Protocol::MoveTeam playerP;
		PlayerRef p = _players[listToPrim[start]];

		ASSERT_CRASH(p != nullptr);

		int32 primid = p->roomprimid;

		p->teamNum = GetTeamNum(primToList[primid]);

		playerP.set_roomprimeid(primid);
		playerP.set_roomlistid(primToList[primid]);
		playerP.set_moveteamnum(p->teamNum);
		informV.push_back(playerP);

	}
	*/
	return informV;
}

void Room::ListSortFromZero()
{
	WRITE_LOCK;
	int32 listsize = ROOMMAX - _ListReuseListID.size();
	int32 lowListID = 1;
	while (true)
	{
		for (int32 i = lowListID; i <= 8; i++)
		{
			if (listToPrim.find(i) == listToPrim.end()) // 비어있다
			{
				if (i > listsize)
					return;
				lowListID = i;

				for (int32 j = lowListID + 1; j <= 8; j++)
				{
					if (listToPrim.find(j) != listToPrim.end())
					{
						cout << "ListSortFromZero : " << j << " : " << lowListID << " : " << listToPrim[j] << endl;
						SetListID(j, lowListID, listToPrim[j]);
						lowListID = i + 1;
						break;
					}
				}
			}
			else lowListID = i + 1;
			
		}
		
	}
}

void RoomShard::Make(RoomRef room)
{
	//_rooms.insert(make_pair(room->GetRoomID(), room));
	WRITE_LOCK;
	_rooms[room->GetRoomID()] = room;
}

void RoomShard::Remove(RoomRef room)
{
	WRITE_LOCK;
	_rooms.erase(room->GetRoomID());
}

void RoomShard::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto& p : _rooms)
	{
		p.second->Broadcast(sendBuffer);
	}
}

RoomRef RoomShard::GetRoom(int32 id)
{
	READ_LOCK;
	auto room = _rooms.find(id);
	if (room == _rooms.end()) // 존재하지않음
		return nullptr;

	return (room->second);
}
void Rooms::Make(RoomRef room)
{
//_rooms.insert(make_pair(room->GetRoomID(), room));
	_roomShards[room->GetRoomID() % SHARD_COUNT].Make(room);
}

void Rooms::Remove(RoomRef room)
{
	_roomShards[room->GetRoomID() % SHARD_COUNT].Remove(room);
}

void Rooms::Broadcast(SendBufferRef sendBuffer)
{
	for (int i = 0; i < SHARD_COUNT; i++)
	{
		_roomShards[i].Broadcast(sendBuffer);
	}
}

RoomRef Rooms::GetRoom(int32 id)
{
	
	return _roomShards[id % SHARD_COUNT].GetRoom(id);
}