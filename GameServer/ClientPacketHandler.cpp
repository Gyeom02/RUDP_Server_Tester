#include "pch.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "Room.h"
#include "PlayerManager.h"
#include "QoSCore.h"
PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 직접 컨텐츠 작업자

bool Handle_C_RUDPACK(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_RUDPACK& pkt)
{
	int32 bhascount = pkt.bhascount();
	int32 start = pkt.start();
	int32 count = pkt.count();
	
	PlayerRef player = GPlayerManager.GetPlayer(pkt.playerid());
	if (player == nullptr)
	{
		//cout << "Handle_C_RUDPACK PlayerID : " << pkt.playerid() << " | SequenceStart : " << pkt.start() << endl;
		return false;
	}
	player->GetDeliveyManager()->ProcessAcks(start, count, bhascount);
	//cout << "Handle_C_RUDPACK PlayerID : " << pkt.playerid() << " | SequenceStart : " << pkt.start() << endl;
	return true;
}

bool Handle_C_DISCONNECT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_DISCONNECT& pkt)
{
	cout << "GET DISCONNECT PKT By : " << pkt.id() << endl;
	int32 id = pkt.id();
	int32 roomid = pkt.roomid();
	int32 roomprimid = pkt.roomprimid();
	//PlayerManager Release
	GPlayerManager.Remove(id);
	// ID Reuse Setting
	GPlayerManager.PushID(id);
	GQoS->ErasePlayer(id);
	//Room Release
	if (roomid == -1) // 방에 들어가있지 않음
	{

	}
	else 
	{ 
		//방에 들어가 있음 (호스트인지 아닌지, 인게임 중인지 아닌지에 따라 달라짐(?)
		
		/*---------------------------------------------------------------------[*/
		
		
		RoomRef room = GRoom->GetRoom(roomid);
		PlayerRef player = room->GetPlayer(id);
		if (!room) // 방이 존재하지 않음
			return true;
		if (!player) //방에 해당 아이디를 가진 플레이어가 없음
		{
			cout << "Room " << roomid << " : Player ID : " << id << " Dose Not Entered" << endl;
			return true;
		}
		room->Leave(player);
			
	}
	
	return true;
}
bool Handle_INVALID(UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}
bool Handle_C_INIT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_INIT& pkt)
{
	static Atomic<int32> idGenerator = 0;
	int32 reuse_id = GPlayerManager.ReuseID();
	int32 id = -1;
	if (reuse_id == -1) // 재사용 가능한  ID가 없음 새로 발급
		id = ++idGenerator;
	else //재사용
	{
		id = reuse_id;
	}
	
	
	PlayerRef playerRef = MakeShared<Player>(id);
	//nickname 임시임
	playerRef->SetNickName(to_string(id));
	//nickname 임시임
	playerRef->netAddress = clientAddr;
	playerRef->ownerSocket = udpSocket;

	GPlayerManager.Add(id, playerRef);

	GQoS->GetShard(id)->MakeQoSPlayer(id);

	if (GPlayerManager.GetPlayer(id) == nullptr)
		cout << "NNULPTR" << endl;
	else cout << id << endl;

	Protocol::S_INIT sendpkt;

	sendpkt.set_id(id);
	sendpkt.set_nickname(playerRef->GetNickName());

	auto sendBuffer = ClientPacketHandler::MakeUnReliableBuffer(sendpkt);

	udpSocket->Send(playerRef, sendBuffer);

	return true;
}
bool Handle_C_LOGIN(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_LOGIN& pkt)
{
	//GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//// TODO : Validation 체크

	//Protocol::S_LOGIN loginPkt;
	//loginPkt.set_bsuccess(1);

	//// DB에서 플레이 정보를 긁어온다
	//// GameSession에 플레이 정보를 저장 (메모리)

	//// ID 발급 (DB 아이디가 아니고, 인게임 아이디)
	

	//{
	//	auto player = loginPkt.add_players();
	//	player->set_name(u8"DB에서긁어온이름1");
	//	player->set_playertype(Protocol::PLAYER_TYPE_KNIGHT);

	//	PlayerRef playerRef = MakeShared<Player>();
	//	playerRef->playerId = idGenerator++;
	//	playerRef->name = player->name();
	//	playerRef->type = player->playertype();
	//	playerRef->ownerSession = gameSession;

	//	gameSession->_players.push_back(playerRef);
	//}

	//{
	//	auto player = loginPkt.add_players();
	//	player->set_name(u8"DB에서긁어온이름2");
	//	player->set_playertype(Protocol::PLAYER_TYPE_MAGE);

	//	PlayerRef playerRef = MakeShared<Player>();
	//	playerRef->playerId = idGenerator++;
	//	playerRef->name = player->name();
	//	playerRef->type = player->playertype();
	//	playerRef->ownerSession = gameSession;

	//	gameSession->_players.push_back(playerRef);
	//}

	//auto sendBuffer = ClientPacketHandler::MakeSendBuffer(loginPkt);
	//session->Send(sendBuffer);

	return true;
}

bool Handle_C_ENTER_GAME(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_ENTER_GAME& pkt)
{
	//GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//uint64 index = pkt.playerindex();
	//// TODO : Validation

	//gameSession->_currentPlayer = gameSession->_players[index]; // READ_ONLY?
	//gameSession->_room = GRoom;

	//GRoom->DoAsync(&Room::Enter, gameSession->_currentPlayer);

	//Protocol::S_ENTER_GAME enterGamePkt;
	//enterGamePkt.set_success(true);
	//auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterGamePkt);
	//gameSession->_currentPlayer->ownerSession->Send(sendBuffer);

	return true;
}

bool Handle_C_MSG(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_MSG& pkt)
{
	//std::cout << pkt.msg() << "^^" << endl;

	//Protocol::S_MSG chatPkt;
	//chatPkt.set_msg(pkt.msg());
	//auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);
	//
	////GRoom->DoAsync(&Room::Broadcast, sendBuffer);
	//udpSocket->Send(clientAddr, sendBuffer);
	return true;
}
bool Handle_C_MAKEROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_MAKEROOM& pkt)
{
	if (GRoom->GetRoom(pkt.id()) != nullptr) //방이 존재한다
	{
		cout << "Room " << pkt.id() << " Already Exist" << endl; 
		return true;
	}
	RoomRef room = make_shared<Room>(pkt.id());
	PlayerRef player = GPlayerManager.GetPlayer(pkt.id());
	player->roomprimid = room->GivePrimID();
	int32 givenlistid = room->FirstGiveListID(player->roomprimid);
	player->teamNum = room->GetTeamNum(givenlistid);

	
	GRoom->Make(room);
	/*if (GPlayerManager.GetPlayer(pkt.id()) == nullptr)
		cout << "NULPPTR" << endl;*/
	room->Enter(player);
		

	Protocol::S_MAKEROOM sendpkt;
	sendpkt.set_roomid(pkt.id());
	sendpkt.set_roomprimid(player->roomprimid);
	sendpkt.set_roomlistid(givenlistid);
	auto sendBuffer = ClientPacketHandler::MakeReliableBuffer(sendpkt, QoSCore::MEDIUM);
	udpSocket->Send(player, sendBuffer);
	//playerRef->type = player->playertype();
	//playerRef->ownerSession = gameSession;
	
	//gameSession->_players.push_back(playerRef);
	

	return true;
}

bool Handle_C_ENTERROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_ENTERROOM& pkt)
{
	/* 방의 존재, 방의 플레이어들 수를 확인하고 클라이언트를 추가하는 코드*/

	RoomRef room = GRoom->GetRoom(pkt.roomid());
	if (room == nullptr)
	{
		//방이 존재하지않음
		cout << "Room : " << pkt.roomid() << " Does Not Exist" << endl;
		return true;
	}
	int32 givenroomid = room->GivePrimID();
		
	if (givenroomid == -1) // 방에 플레이어 수가 8명 꽉참
	{
		cout << "Room : " << pkt.roomid() << " Fulled" << endl;
		return true;
	}
	PlayerRef player = GPlayerManager.GetPlayer(pkt.id());
	if (player == nullptr)
	{
		//플레이어가 존재하지않음
		cout << pkt.id() << " : Player Not Existed" << endl;
		return true;
	}
	player->roomprimid = givenroomid;
	int32 givenlistid = room->FirstGiveListID(givenroomid);
	player->teamNum = room->GetTeamNum(givenlistid);
	//room->SetListID(-1, givenlistid, givenroomid);

	/* 기존 존재하던 플레이어들에게 지금 들어가는 플레이어의 정보의 패킷을 보내는 코드*/
	Protocol::S_NEWPLAYER newplayerpkt;
	auto pktplayer = newplayerpkt.mutable_players();
	pktplayer->set_id(pkt.id());
	//닉네임 임시임
	pktplayer->set_name(to_string(pkt.id()));
	pktplayer->set_roomid(givenroomid); // roomprimid를 의미함
	pktplayer->set_roomlistid(givenlistid); // roomlistid를 의미함
	SendBufferRef newplayersb = ClientPacketHandler::MakeReliableBuffer(newplayerpkt, QoSCore::HIGH);
	room->Broadcast(newplayersb);
	/* 지금 들어가는 플레이어에게 기존 존재하던 플레이어들의 정보를 동기화 및 방에 진입하게 하는 패킷을 보내는 코드*/
	Protocol::S_ENTERROOM enterpkt;
		
	enterpkt.set_roomid(pkt.roomid());
	enterpkt.set_roomprimid(givenroomid);
	enterpkt.set_roomlistid(givenlistid);
	enterpkt.set_teammode(room->GetTeamMode());
	for (auto p : room->GetPlayers())
	{
		PlayerRef ps = p.second;
		auto pktplayers = enterpkt.add_players();
		pktplayers->set_id(ps->playerId);
		//닉네임 임시임
		pktplayers->set_name(to_string(ps->playerId));
		pktplayers->set_roomid(ps->roomprimid);
		pktplayers->set_roomlistid(room->GetPrimToList(ps->roomprimid));
	}
	auto entersendbuffer = ClientPacketHandler::MakeReliableBuffer(enterpkt, QoSCore::HIGH);
	udpSocket->Send(player, entersendbuffer);
	room->Enter(player);
		
	
	return true;
}

bool Handle_C_MOVETEAM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_MOVETEAM& pkt)
{
	
	RoomRef room = GRoom->GetRoom(pkt.roomid());
		
	if (room)
	{
		PlayerRef player = room->GetPlayer(pkt.roomprimeid());
		if (player == nullptr)
		{
			cout << "Player Nulltpr : HANDLE_C_MOVETEAM " << endl;
			return true;
		}
		if (player->teamNum == room->GetTeamNum(pkt.movetolistid()))
		{
			cout << "Same Team Can Not Change Team " << endl;
			return true;
		}
		bool bWhoTaken = room->SetListID(pkt.roomlistid(), pkt.movetolistid(), pkt.roomprimeid());
		if (!bWhoTaken) //누군가 이미 갖고있음
		{
			cout << "누군가 이미 갖고있음" << endl;
			return true;
		}
			
		player->teamNum = room->GetTeamNum(pkt.movetolistid());

		Protocol::S_MOVETEAM movepkt;

		Protocol::MoveList* imform = movepkt.add_moveinforms();
		imform->set_roomprimeid(pkt.roomprimeid());
		imform->set_roomlistid(pkt.roomlistid());
		imform->set_movetolistid(pkt.movetolistid());
		imform->set_moveteamnum(player->teamNum);
		auto sendBuffer = ClientPacketHandler::MakeReliableBuffer(movepkt, QoSCore::HIGH);
		room->Broadcast(sendBuffer);
			
		room->ListSort(room->GetTeamNum(pkt.roomlistid()), pkt.roomlistid());

			
				
			
		/*int32 roomprimeid = 1;
int32 roomdlistid = 2;
int32 movetolistid = 3;*/

	}
		
	return true;
}

bool Handle_C_LEAVEROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_LEAVEROOM& pkt) // 살펴봐야함
{

	
	int32 nextHostID = -1;
	bool bEraseRoom = false;
	RoomRef room = GRoom->GetRoom(pkt.roomid());
	if (room == nullptr)
		return true;
	PlayerRef player = room->GetPlayer(pkt.primid());
	if (player == nullptr)
		return true;

	if (player->playerId == room->GetRoomID()) //방에 주인인데 나가려고 함
	{
	
		if (room->PlayerNum() == 1)// 방에 나밖에 없다
		{
			
			bEraseRoom = true;
		}
		else
		{   //방 양도 하는 코드
			//Host 체인지
			PlayerRef newHostplayer = nullptr;
			for (auto players : room->GetPlayers())
			{
				newHostplayer = players.second;
				if (newHostplayer != player) //Host Player가 아닌 다른 플레이어
					break;
			}
			nextHostID = newHostplayer->playerId;

			//GRoom에서 방 Key Value 변경된 Host ID로 체인지
			GRoom->Remove(room);
			room->SetRoomID(nextHostID);
			GRoom->Make(room);
			for (auto p : room->GetPlayers())
			{
				PlayerRef pp = p.second;
				pp->roomId = nextHostID;
			}
			//방에 있는 플레이어의 RoomID 다 체인지
		}

	}
	

	int32 leaveListID = room->GetPrimToList(player->roomprimid);
	Protocol::S_LEAVEROOM sendpkt;
	sendpkt.set_primid(player->roomprimid);
	sendpkt.set_listid(room->GetPrimToList(player->roomprimid));
	sendpkt.set_newroomid(nextHostID);
	SendBufferRef sendBuffer = ClientPacketHandler::MakeReliableBuffer(sendpkt, QoSCore::HIGH);
	room->Broadcast(sendBuffer);
	if (!bEraseRoom)
	{
		room->Leave(player);

		room->ListSort(room->GetTeamNum(leaveListID), leaveListID);
	}
	else
	{
		GRoom->Remove(room);
	}
		
	
	return true;
}

bool Handle_C_CHANGETEAMMODE(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_CHANGETEAMMODE& pkt)
{
	
	RoomRef room = GRoom->GetRoom(pkt.roomid());
	if (room)
	{
		if (room->GetRoomID() == pkt.id()) // 보낸 클라이언트가 방의 Host가 맞음
		{
			room->SetTeamMode(pkt.teammode());

			Protocol::S_CHANGETEAMMODE sendpkt;
			sendpkt.set_teammode(pkt.teammode());
			auto v = room->ResetTeamNum();
			while (!v.empty())
			{
				Protocol::MoveList* mt = sendpkt.add_moveteams();
				*mt = v.back();
				v.pop_back();
			}
			SendBufferRef sendbuffer = ClientPacketHandler::MakeReliableBuffer(sendpkt, QoSCore::HIGH);
			room->Broadcast(sendbuffer);
		}
	}
	
	return true;
}


bool Handle_C_MOVESELECTROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_MOVESELECTROOM& pkt)
{
	if (pkt.id() == pkt.roomid()) // 방의 호스트이다
	{
		
		RoomRef room = GRoom->GetRoom(pkt.roomid());
		if (room)
		{
			room->SetRoomFlow(Room::SELECTROOM);
			Protocol::S_MOVESELECTROOM pktt;
			SendBufferRef sendbuffer = ClientPacketHandler::MakeReliableBuffer(pktt, QoSCore::MEDIUM);
			room->Broadcast(sendbuffer);
		}
		
	}

	return true;
}

bool Handle_C_CHANGECHARAC(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_CHANGECHARAC& pkt)
{
	
	RoomRef room = GRoom->GetRoom(pkt.roomid());
	if (room)
	{
		Protocol::S_CHANGECHARAC pktt;
		pktt.set_primid(pkt.primid());
		pktt.set_characid(pkt.characid());
		pktt.set_skinid(pkt.skinid());
		SendBufferRef sendbuffer = ClientPacketHandler::MakeReliableBuffer(pktt, QoSCore::HIGH);
		room->Broadcast(sendbuffer);

	}
		
	return false;
}

bool Handle_C_READY(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_READY& pkt)
{
	
	RoomRef room = GRoom->GetRoom(pkt.roomid());
	if (room)
	{
		if (room->GetRoomFlow() == Room::SELECTROOM)
		{
			PlayerRef player = room->GetPlayer(pkt.primid());
			if (!player)
				return true;
			if (player->bready) // 이미 레디를 박은 상태이다 -> 레디를 풀고싶다
			{
				player->bready = false;
				room->MinusReady();
			}
			else
			{
				player->bready = true;
				room->AddReady();

				if (room->GetReady() == room->PlayerNum())// 모두 레디를 눌렀다
				{
					room->SetRoomFlow(Room::INGAME);
					room->ResetReady();
					for (auto& player : room->GetPlayers())
					{
						PlayerRef p = player.second;
						if (p)
						{
							p->bready = false;
						}
					}
					Protocol::S_STARTGAME startpkt;
					SendBufferRef spsendbuffer = ClientPacketHandler::MakeReliableBuffer(startpkt, QoSCore::HIGH);
					room->Broadcast(spsendbuffer);
			
				}
				else
				{
						
				}

			}
			Protocol::S_READY readypkt;
			readypkt.set_primid(pkt.primid());
			readypkt.set_bready(player->bready);
			SendBufferRef rpsendbuffer = ClientPacketHandler::MakeReliableBuffer(readypkt, QoSCore::HIGH);
			room->Broadcast(rpsendbuffer);
		}
	}
	
	return false;
}


bool Handle_C_SENDIMPORT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header, Protocol::C_SENDIMPORT& pkt)
{
	//cout << "Handle_C_SENDIMPORT" << endl;
	auto moveImport = pkt.playermove();
	RoomRef room = GRoom->GetRoom(moveImport.roomid());
	//cout << "!room" << endl;
	if (room)
	{
		//cout << "room" << endl;
		
		PlayerRef player = room->GetPlayer(moveImport.primid());
		if (player)
		{
			//cout << "player" << endl;
			auto _pos = moveImport.pos();
			auto _rot = moveImport.rot();
			auto _vel = moveImport.vel();
			player->SetImport(Position(_pos.x(), _pos.y(), _pos.z()), Rotation(_rot.pitch(), _rot.yaw(), _rot.roll()), Velocity(_vel.vx(), _vel.vy(), _vel.vz()));

			Protocol::S_SENDIMPORT pktt;
			for (auto& p : room->GetPlayers())
			{
				//cout << "auto& p : room->GetPlayers()" << endl;
				PlayerRef& player = p.second;
				auto imform = pktt.add_playermoves();
				imform->set_primid(player->roomprimid);
				auto pktpos = imform->mutable_pos();
				auto pktrot = imform->mutable_rot();
				auto pktvel = imform->mutable_vel();
				pktpos->set_x(player->pos.x);
				pktpos->set_y(player->pos.y);
				pktpos->set_z(player->pos.z);
				pktrot->set_pitch(player->rot.pitch);
				pktrot->set_yaw(player->rot.yaw);
				pktrot->set_roll(player->rot.roll);
				pktvel->set_vx(player->vel.vx);
				pktvel->set_vy(player->vel.vy);
				pktvel->set_vz(player->vel.vz);

			}
			SendBufferRef sendbuffer = ClientPacketHandler::MakeReliableBuffer(pktt, QoSCore::HIGH);
			udpSocket->Send(player, sendbuffer);
		}
	}
	/*GRoom->DoAsync([=]() {
		
		});*/
	return true;
}

