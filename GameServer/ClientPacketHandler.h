#pragma once
#include "Protocol.pb.h"
#include "UDP.h"
#include "NetAddress.h"
//#include "DeliveryNotificationManager.h"
#include "Player.h"
#include "QoSCore.h"

using PacketHandlerFunc = std::function<bool(UDPSocketPtr, NetAddress, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C_DISCONNECT = 1000,
	PKT_S_DISCONNECT = 1001,
	PKT_C_INIT = 1002,
	PKT_S_INIT = 1003,
	PKT_C_LOGIN = 1004,
	PKT_S_LOGIN = 1005,
	PKT_C_ENTER_GAME = 1006,
	PKT_S_ENTER_GAME = 1007,
	PKT_C_MSG = 1008,
	PKT_S_MSG = 1009,
	PKT_C_MAKEROOM = 1010,
	PKT_S_MAKEROOM = 1011,
	PKT_C_ENTERROOM = 1012,
	PKT_S_ENTERROOM = 1013,
	PKT_S_NEWPLAYER = 1014,
	PKT_C_MOVETEAM = 1015,
	PKT_S_MOVETEAM = 1016,
	PKT_C_LEAVEROOM = 1017,
	PKT_S_LEAVEROOM = 1018,
	PKT_C_CHANGETEAMMODE = 1019,
	PKT_S_CHANGETEAMMODE = 1020,
	PKT_C_MOVESELECTROOM = 1021,
	PKT_S_MOVESELECTROOM = 1022,
	PKT_C_CHANGECHARAC = 1023,
	PKT_S_CHANGECHARAC = 1024,
	PKT_C_READY = 1025,
	PKT_S_READY = 1026,
	PKT_S_STARTGAME = 1027,
	PKT_C_SENDIMPORT = 1028,
	PKT_S_SENDIMPORT = 1029,
	PKT_C_RUDPACK = 1030,
	PKT_S_RUDPACK = 1031,
};

// Custom Handlers
bool Handle_INVALID(UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len);
bool Handle_C_DISCONNECT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_DISCONNECT& pkt);
bool Handle_C_INIT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_INIT& pkt);
bool Handle_C_LOGIN(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_LOGIN& pkt);
bool Handle_C_ENTER_GAME(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_ENTER_GAME& pkt);
bool Handle_C_MSG(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_MSG& pkt);
bool Handle_C_MAKEROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_MAKEROOM& pkt);
bool Handle_C_ENTERROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_ENTERROOM& pkt);
bool Handle_C_MOVETEAM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_MOVETEAM& pkt);
bool Handle_C_LEAVEROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_LEAVEROOM& pkt);
bool Handle_C_CHANGETEAMMODE(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_CHANGETEAMMODE& pkt);
bool Handle_C_MOVESELECTROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_MOVESELECTROOM& pkt);
bool Handle_C_CHANGECHARAC(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_CHANGECHARAC& pkt);
bool Handle_C_READY(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_READY& pkt);
bool Handle_C_SENDIMPORT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_SENDIMPORT& pkt);
bool Handle_C_RUDPACK(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::C_RUDPACK& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_DISCONNECT] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_DISCONNECT>(Handle_C_DISCONNECT, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_INIT] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_INIT>(Handle_C_INIT, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_LOGIN] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LOGIN>(Handle_C_LOGIN, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_GAME>(Handle_C_ENTER_GAME, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_MSG] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MSG>(Handle_C_MSG, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_MAKEROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MAKEROOM>(Handle_C_MAKEROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_ENTERROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTERROOM>(Handle_C_ENTERROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_MOVETEAM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVETEAM>(Handle_C_MOVETEAM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_LEAVEROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LEAVEROOM>(Handle_C_LEAVEROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_CHANGETEAMMODE] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHANGETEAMMODE>(Handle_C_CHANGETEAMMODE, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_MOVESELECTROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVESELECTROOM>(Handle_C_MOVESELECTROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_CHANGECHARAC] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHANGECHARAC>(Handle_C_CHANGECHARAC, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_READY] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_READY>(Handle_C_READY, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_SENDIMPORT] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_SENDIMPORT>(Handle_C_SENDIMPORT, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_C_RUDPACK] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_RUDPACK>(Handle_C_RUDPACK, udpSocket, clientAddr, buffer, len); };
	}

	static bool HandlePacket(UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](udpSocket, clientAddr, buffer, len);
	}

private:
	static SendBufferRef MakeSendBuffer(Protocol::S_DISCONNECT& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_DISCONNECT, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_INIT& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_INIT, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LOGIN& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_LOGIN, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTER_GAME& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_ENTER_GAME, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MSG& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_MSG, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MAKEROOM& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_MAKEROOM, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTERROOM& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_ENTERROOM, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_NEWPLAYER& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_NEWPLAYER, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVETEAM& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_MOVETEAM, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LEAVEROOM& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_LEAVEROOM, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHANGETEAMMODE& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_CHANGETEAMMODE, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVESELECTROOM& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_MOVESELECTROOM, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHANGECHARAC& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_CHANGECHARAC, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_READY& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_READY, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_STARTGAME& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_STARTGAME, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SENDIMPORT& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_SENDIMPORT, breliable, class_traffic); }
	static SendBufferRef MakeSendBuffer(Protocol::S_RUDPACK& pkt,const bool& breliable= true, const uint16& class_traffic = 0) { return MakeSendBuffer(pkt, PKT_S_RUDPACK, breliable, class_traffic); }
	

	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(udpSocket, clientAddr, header, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId, const bool& breliable, const uint16& class_traffic) 
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		header->breliable = breliable;
		header->priority = class_traffic;
		header->retransnum = 0;
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}

public:

	template<typename PKT>
	static SendBufferRef MakeReliableBuffer(PKT pkt, uint16 priority) { _ASSERT(priority < QoSCore::MAX);  return MakeSendBuffer(pkt, true, priority); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer(PKT pkt) { return MakeSendBuffer(pkt, false, QoSCore::FPC); } //
};