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
bool Handle_S_DISCONNECT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_DISCONNECT& pkt);
bool Handle_S_INIT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_INIT& pkt);
bool Handle_S_LOGIN(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_LOGIN& pkt);
bool Handle_S_ENTER_GAME(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_ENTER_GAME& pkt);
bool Handle_S_MSG(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_MSG& pkt);
bool Handle_S_MAKEROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_MAKEROOM& pkt);
bool Handle_S_ENTERROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_ENTERROOM& pkt);
bool Handle_S_NEWPLAYER(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_NEWPLAYER& pkt);
bool Handle_S_MOVETEAM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_MOVETEAM& pkt);
bool Handle_S_LEAVEROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_LEAVEROOM& pkt);
bool Handle_S_CHANGETEAMMODE(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_CHANGETEAMMODE& pkt);
bool Handle_S_MOVESELECTROOM(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_MOVESELECTROOM& pkt);
bool Handle_S_CHANGECHARAC(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_CHANGECHARAC& pkt);
bool Handle_S_READY(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_READY& pkt);
bool Handle_S_STARTGAME(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_STARTGAME& pkt);
bool Handle_S_SENDIMPORT(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_SENDIMPORT& pkt);
bool Handle_S_RUDPACK(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::S_RUDPACK& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_S_DISCONNECT] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DISCONNECT>(Handle_S_DISCONNECT, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_INIT] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_INIT>(Handle_S_INIT, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_LOGIN] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN>(Handle_S_LOGIN, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_ENTER_GAME] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_GAME>(Handle_S_ENTER_GAME, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_MSG] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MSG>(Handle_S_MSG, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_MAKEROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MAKEROOM>(Handle_S_MAKEROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_ENTERROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTERROOM>(Handle_S_ENTERROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_NEWPLAYER] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_NEWPLAYER>(Handle_S_NEWPLAYER, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_MOVETEAM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVETEAM>(Handle_S_MOVETEAM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_LEAVEROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LEAVEROOM>(Handle_S_LEAVEROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_CHANGETEAMMODE] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANGETEAMMODE>(Handle_S_CHANGETEAMMODE, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_MOVESELECTROOM] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVESELECTROOM>(Handle_S_MOVESELECTROOM, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_CHANGECHARAC] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANGECHARAC>(Handle_S_CHANGECHARAC, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_READY] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_READY>(Handle_S_READY, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_STARTGAME] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_STARTGAME>(Handle_S_STARTGAME, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_SENDIMPORT] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SENDIMPORT>(Handle_S_SENDIMPORT, udpSocket, clientAddr, buffer, len); };
		GPacketHandler[PKT_S_RUDPACK] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_RUDPACK>(Handle_S_RUDPACK, udpSocket, clientAddr, buffer, len); };
	}

	static bool HandlePacket(UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](udpSocket, clientAddr, buffer, len);
	}

private:
	static SendBufferRef MakeSendBuffer(Protocol::C_DISCONNECT& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_DISCONNECT, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_INIT& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_INIT, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_LOGIN& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_LOGIN, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_ENTER_GAME& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_ENTER_GAME, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MSG& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_MSG, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MAKEROOM& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_MAKEROOM, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_ENTERROOM& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_ENTERROOM, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MOVETEAM& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_MOVETEAM, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_LEAVEROOM& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_LEAVEROOM, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHANGETEAMMODE& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_CHANGETEAMMODE, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MOVESELECTROOM& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_MOVESELECTROOM, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHANGECHARAC& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_CHANGECHARAC, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_READY& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_READY, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_SENDIMPORT& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_SENDIMPORT, channel, priorty); }
	static SendBufferRef MakeSendBuffer(Protocol::C_RUDPACK& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_C_RUDPACK, channel, priorty); }
	

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
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId, const uint16& channel, const uint16& priorty)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		header->channel = channel;
		header->priority = priorty;
		header->bFragment = 0;
		header->controlflag = 0;
		header->retransnum = 0;
		header->sent_timestamp = 0;
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
	template<typename PKT>
	static SendBufferRef MakeReliableBuffer(PKT pkt, uint16 priority) { return MakeSendBuffer(pkt, QoS::Channel::RO, priority); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer(PKT pkt, uint16 priority) { return MakeSendBuffer(pkt, QoS::Channel::URO, priority); }
	
public:

	template<typename PKT>
	static SendBufferRef MakeReliableBuffer_High(PKT pkt) { return MakeReliableBuffer(pkt, QoS::Priority::HIGH); }
	template<typename PKT>
	static SendBufferRef MakeReliableBuffer_Medium(PKT pkt) { return MakeReliableBuffer(pkt, QoS::Priority::MEDIUM); }
	template<typename PKT>
	static SendBufferRef MakeReliableBuffer_Low(PKT pkt) { return MakeReliableBuffer(pkt, QoS::Priority::LOW); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer_High(PKT pkt) { return MakeUnReliableBuffer(pkt, QoS::Priority::HIGH); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer_Medium(PKT pkt) { return MakeUnReliableBuffer(pkt, QoS::Priority::MEDIUM); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer_Low(PKT pkt) { return MakeUnReliableBuffer(pkt, QoS::Priority::LOW); }

	template<typename PKT>
	static SendBufferRef MakeReliableBuffer(PKT pkt) { return MakeReliableBuffer_Medium(pkt); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer(PKT pkt) { return MakeUnReliableBuffer_Medium(pkt); }

	template<typename PKT>
	static SendBufferRef MakeReplicateBuffer(PKT pkt) { return MakeSendBuffer(pkt, QoS::Channel::RPCT, QoS::Priority::LOW); }
	
};