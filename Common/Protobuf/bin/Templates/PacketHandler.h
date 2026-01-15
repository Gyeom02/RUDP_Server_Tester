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
{%- for pkt in parser.total_pkt %}
	PKT_{{pkt.name}} = {{pkt.id}},
{%- endfor %}
};

// Custom Handlers
bool Handle_INVALID(UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len);
{%- for pkt in parser.recv_pkt %}
bool Handle_{{pkt.name}}(UDPSocketPtr udpSocket, NetAddress clientAddr, PacketHeader* header,  Protocol::{{pkt.name}}& pkt);
{%- endfor %}

class {{output}}
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

{%- for pkt in parser.recv_pkt %}
		GPacketHandler[PKT_{{pkt.name}}] = [](UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE* buffer, int32 len) { return HandlePacket<Protocol::{{pkt.name}}>(Handle_{{pkt.name}}, udpSocket, clientAddr, buffer, len); };
{%- endfor %}
	}

	static bool HandlePacket(UDPSocketPtr udpSocket, NetAddress clientAddr, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](udpSocket, clientAddr, buffer, len);
	}

private:
		
{%- for pkt in parser.send_pkt %}
	static SendBufferRef MakeSendBuffer(Protocol::{{pkt.name}}& pkt, const uint16& channel, const uint16& priorty) { return MakeSendBuffer(pkt, PKT_{{pkt.name}}, channel, priorty); }
{%- endfor %}
	

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
		header->retransnum = 0;
		header->controlflag = 0;
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}

public:

	template<typename PKT>
	static SendBufferRef MakeReliableBuffer(PKT pkt, uint16 priorty) {  return MakeSendBuffer(pkt, QoSCore::Channel::RO, priorty); }
	template<typename PKT>
	static SendBufferRef MakeUnReliableBuffer(PKT pkt) { return MakeSendBuffer(pkt, QoSCore::Channel::URO, QoSCore::Priority::LOW); } //
	template<typename PKT>
	static SendBufferRef MakeReplicateBuffer(PKT pkt) { return MakeSendBuffer(pkt, QoSCore::Channel::RPCT, QoSCore::Priority::LOW); } //

	
};