#include "pch.h"
#include "ServerPacketHandler.h"
#include "Player.h"
#include "PlayerManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 직접 컨텐츠 작업자

bool Handle_S_RUDPACK(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_RUDPACK& pkt)
{
	//cout << "Handle_S_RUDPACK" << endl;
	int32 bhascount = pkt.bhascount();
	int32 start = pkt.start();
	int32 count = pkt.count();
	/*if (pkt.playerid() == 0)
		return false;*/
	PlayerRef player = GPlayerManager.GetPlayer(header->playerId);
	//cout << "Handle_S_RUDPACK PlayerID : " << header->playerId << endl;
 	player->GetDeliveryManager()->ProcessAcks(start, count, bhascount);
	return true;
}

bool Handle_INVALID(UDPSocketPtr udpSocket, NetAddress netAddress, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}
bool Handle_S_DISCONNECT(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_DISCONNECT& pkt)
{
	return false;
}
bool Handle_S_INIT(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_INIT& pkt)
{
	PlayerRef _player = MakeShared<Player>(pkt.id());
	_player->ownerSocket = GUDP.GetUDPSocket(0)->shared_from_this();
	_player->netAddress = _player->ownerSocket->GetNetAddress();
	GPlayerManager.Add(pkt.id(), _player);
	
	GQoS->GetShard(pkt.id())->MakeQoSPlayer(pkt.id());
	
	//PlayerRef player = GPlayerManager.GetPlayer();
	
	_player->playerId.store(int32(pkt.id()));
	cout << "Handle_S_INIT : " << _player->playerId.load() << endl;
	return true;
}
bool Handle_S_LOGIN(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_LOGIN& pkt)
{
	//if (pkt.success() == false)
	//	return true;

	//if (pkt.players().size() == 0)
	//{
	//	// 캐릭터 생성창
	//}

	//// 입장 UI 버튼 눌러서 게임 입장
	//Protocol::C_ENTER_GAME enterGamePkt;
	//enterGamePkt.set_playerindex(0); // 첫번째 캐릭터로 입장
	//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
	//session->Send(sendBuffer);

	return true;
}
bool Handle_S_ENTER_GAME(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_ENTER_GAME& pkt)
{
	return false;
}
bool Handle_S_MSG(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_MSG& pkt)
{
	//std::cout << pkt.msg() << endl;
//	GPlayerManager._recvPacketNum++;
	return true;
}
bool Handle_S_MAKEROOM(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_MAKEROOM& pkt)
{
	return false;
}
bool Handle_S_ENTERROOM(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_ENTERROOM& pkt)
{
	return false;
}
bool Handle_S_NEWPLAYER(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_NEWPLAYER& pkt)
{
	return false;
}
bool Handle_S_MOVETEAM(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_MOVETEAM& pkt)
{
	return false;
}
bool Handle_S_LEAVEROOM(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_LEAVEROOM& pkt)
{
	return false;
}
bool Handle_S_CHANGETEAMMODE(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_CHANGETEAMMODE& pkt)
{
	return false;
}
bool Handle_S_MOVESELECTROOM(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_MOVESELECTROOM& pkt)
{
	return false;
}
bool Handle_S_CHANGECHARAC(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_CHANGECHARAC& pkt)
{
	return false;
}
bool Handle_S_READY(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_READY& pkt)
{
	return false;
}
bool Handle_S_STARTGAME(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_STARTGAME& pkt)
{
	return false;
}
bool Handle_S_SENDIMPORT(UDPSocketPtr udpSocket, NetAddress netAddress, PacketHeader* header, Protocol::S_SENDIMPORT& pkt)
{
	return false;
}
