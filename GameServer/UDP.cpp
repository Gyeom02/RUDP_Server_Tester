#include "pch.h"
#include "UDP.h"


#define SERVERADDR L"192.168.219.102"

UDP GUDP;
bool UDP::UDPSocketReset(int32 index)
{
	UDPSocketPtr object = MakeShared<UDPSocket>();

	_udpSockets[index] = object; //UDP Socket들을 모아놓고 저장하는 Array에 넣는 줄

	SOCKET& _udpSocket = object->GetSocket();
	/*if (_udpSocket != INVALID_SOCKET)
		return false;*/
	_udpSocket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_udpSocket == INVALID_SOCKET)
	{
		cout << "Failed Creating UDP Socket : " << index << endl;
		return false;
	}
	
	SOCKADDR_IN udpAddr;
	::memset(&udpAddr, 0, sizeof(udpAddr));
	udpAddr.sin_family = AF_INET;
	udpAddr.sin_port = ::htons(7777 + index);
	IN_ADDR address;
	InetPtonW(AF_INET, SERVERADDR, &address);
	udpAddr.sin_addr = address;


	//int optval = 1;
	//setsockopt(_udpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
#ifdef _SERVER
	if (::bind(_udpSocket, (SOCKADDR*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR)
	{
		cout << "Server Socket Bind Failed " << endl;
		return false;
	}
#else
#endif
	object->GetNetAddress() = NetAddress(udpAddr);

	u_long bflag = 1;
	if (::ioctlsocket(object->GetSocket(), FIONBIO, &bflag) == INVALID_SOCKET)
	{
		cout << "Failed UDP Non-Blocking : " << index << endl;
		return false;
	}
	WSAEVENT& _wsaEvent = object->GetWSAEvent();
	_wsaEvent = WSACreateEvent();
	
	if (::WSAEventSelect(_udpSocket, _wsaEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
	{
		cout << "Failed UDP WSAEventSelect : " << index << endl;
		return false;
	}
	cout << "UDP Socket Created : " << index << endl;
	return true;
}

UDP::UDP()
{
	/*if (UDPInit())
	{
		cout << "UDP Init Succeed" << endl;
		_IsUDPOn = true;
	}*/
}

bool UDP::UDPInit()
{
	for (int32 i = 0; i < SOCKNUM; i++)
	{
		if (!UDPSocketReset(i))
		{
			cout << "UDP Init Failed" << endl;
			return false;
		}
	}
	return true;
}

void UDP::UDPClear()
{
	for (int32 i = 0; i < SOCKNUM; i++)
	{
		UDPSocketPtr object = _udpSockets[i];
		WSACloseEvent(object->GetWSAEvent());
		//cout << "UDP Clear : " << i << endl;
	}
	_udpSockets.fill({});
	_IsUDPOn = false;
}

void UDP::CheckPacketPriority(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	if (header->priority == QoSCore::FPC)
	{
		
		GUDPJob.Push([=]() {
			GUDP.UDPPacketHandle(udpSocket, clientAddress, buffer, header->size);
			}, QoSCore::LOW);
	}
	else
	{
		GUDPJob.Push([=]() {
			GUDP.UDPPacketHandle(udpSocket, clientAddress, buffer, header->size);
			}, QoSCore::HIGH);
	}
}

void UDP::UDPPacketHandle(UDPSocketPtr udpSocket, NetAddress clientAddress, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	ClientPacketHandler::HandlePacket(udpSocket, clientAddress, buffer, len);

	delete[] buffer;
	buffer = nullptr;
	//if (header->id == 1004) // 1004는 임시 패킷 번호(메세지 전송 패킷)이다
	//{

	//	Protocol::C_CHAT pkt;
	//	if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), header->size - sizeof(PacketHeader)) != false)
	//	{
	//		cout << "Client : " << pkt.msg() << endl;
	//	}
	//	/*----------------------------예상 처리 코드-------------------------- - */

	//	/*Protocol::C_SENDMessage pkt;
	//	if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), header->size - sizeof(PacketHeader)) != false)
	//	{
	//		//	SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

	//		SOCKADDR_IN otherClientSockAddr;
	//		otherClientSockAddr.sin_family = AF_INET;

	//		PlayerRef OtherPlayer = nullptr;
	//		if (GRoom->IsExsistPlayer(pkt.roomid(), pkt.opposeid()))
	//		{
	//			OtherPlayer = GRoom->GetRooms()[pkt.roomid()]->GetPlayers()[pkt.opposeid()];
	//		}
	//		
	//		if (OtherPlayer != nullptr)
	//		{
	//			inet_pton(AF_INET, OtherPlayer->udpAddr.c_str(), &otherClientSockAddr.sin_addr);
	//			otherClientSockAddr.sin_port = OtherPlayer->udpPort;

	//			::sendto(udpSocket, (char*)(buffer), len, 0, (SOCKADDR*)&clientAddress, sizeof(clientAddress));
	//		}//::sendto(udpSocket, (char*)(sendBuffer->Buffer()), sendBuffer->WriteSize(), 0, (SOCKADDR*)&otherClientSockAddr, sizeof(otherClientSockAddr));
	//	}*/
	//}
}

void UDP::UDPDoJop()
{
	while (true)
	{
		GUDPJob.DOJob();
	}
}
