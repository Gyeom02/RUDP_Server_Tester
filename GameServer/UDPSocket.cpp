#include "pch.h"
#include "UDPSocket.h"
#include "UDP.h"
#include "PlayerManager.h"
#include "QoSCore.h"

UDPSocket::UDPSocket()
	: udpRecvBuffer(65536)
{

}

UDPSocket::~UDPSocket()
{
}
void UDPSocket::UDPWork()
{
	cout << "Succeed Creating UDP Socket" << endl;
	
	while (true)
	{
	
		int32 index = ::WSAWaitForMultipleEvents(1, &_wsaEvent, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED)
			continue;
		index -= WSA_WAIT_EVENT_0;
	
		WSANETWORKEVENTS networkEvents;
		if (::WSAEnumNetworkEvents(_socket, _wsaEvent, &networkEvents) == SOCKET_ERROR)
			continue;
	
		if (networkEvents.lNetworkEvents & FD_READ)
		{
			if ((networkEvents.lNetworkEvents & FD_READ) && (networkEvents.iErrorCode[FD_READ_BIT] != 0))
				continue;

			CHAR* buffer = reinterpret_cast<char*>(udpRecvBuffer.WritePos());

			SOCKADDR_IN recvAddr;
			::memset(&recvAddr, 0, sizeof(recvAddr));
			int32 addrLen = sizeof(recvAddr);
			int32 recvLen = ::recvfrom(_socket, buffer, udpRecvBuffer.FreeSize(), 0, (SOCKADDR*)&recvAddr, &addrLen);
			if (recvLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
			{
				continue;
			}
			if (recvLen == 0)
			{
				continue;
			}
			if (udpRecvBuffer.OnWrite(recvLen) == false)
			{
				continue;
			}
			int32 dataSize = udpRecvBuffer.DataSize();
			int32 processLen = 0;
			while (true)
			{
				recvLen = dataSize - processLen;
				if (recvLen < sizeof(PacketHeader))
					break;
				PacketHeader* header = reinterpret_cast<PacketHeader*>(&udpRecvBuffer.ReadPos()[processLen]);
				if (header->size > recvLen)
					break;
				//cout << "SERVER GOT MSG FROM : " << header->playerId << endl;
				if (header->breliable)
				{
					PlayerRef player = GPlayerManager.GetPlayer(header->playerId);
					
					if (player->GetDeliveyManager()->ProcessSequenceNumber(header->sn) == false)
					{
						processLen += header->size;
						continue;
					}
				}
				BYTE* cpybuffer = new BYTE[1000];
				::memcpy(cpybuffer, &udpRecvBuffer.ReadPos()[processLen], header->size);
				
				GUDP.CheckPacketPriority(shared_from_this(), NetAddress(recvAddr), cpybuffer, header->size);
				//cout << "SERVER GOT MSG FROM : " << header->playerId << endl;
				processLen += header->size;
			}
			if (processLen < 0 || dataSize < processLen || udpRecvBuffer.OnRead(processLen) == false)
			{
				cout << "DataSize : " << dataSize << " ProcessLen : " << processLen << endl;
				cout << "UDP OnRead Overflow" << endl;
			}
			udpRecvBuffer.Clean();
		}
		if (networkEvents.lNetworkEvents & FD_WRITE)
		{
			if ((networkEvents.lNetworkEvents & FD_WRITE) && (networkEvents.iErrorCode[FD_WRITE_BIT] != 0))
				continue;
		}
		// FD_CLOSE Ã³¸®
		if (networkEvents.lNetworkEvents & FD_CLOSE)
		{
			// TODO : Remove Socket

		}
	}
}

int32 UDPSocket::Send(PlayerRef player, SendBufferRef sendBuffer)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	if (header->priority == QoSCore::FPC)
	{
		FPCSend(player, sendBuffer);
		return 0;
	}
	GQoS->Push(player->playerId, player, sendBuffer);
	return 0;
}


int32 UDPSocket::PriortySend(PlayerRef player, SendBufferRef sendBuffer)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	if (header->priority == QoSCore::FPC)
		return FPCSend(player, sendBuffer);
	if (header->breliable)
		return ReliableSend(player, sendBuffer);
	else
		return UnReliableSend(player, sendBuffer);
}

int UDPSocket::ReliableSend(PlayerRef player, SendBufferRef sendBuffer)
{
	NetAddress netAddr = player->netAddress;
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	//header->playerId = player->playerId;

	player->GetDeliveyManager()->WriteSeqeuenceNumber(player->ownerSocket->GetSocket(), netAddr, sendBuffer);

	return FPCSend(player, sendBuffer);
}

int UDPSocket::UnReliableSend(PlayerRef player, SendBufferRef sendBuffer)
{
	return FPCSend(player, sendBuffer);
}

int UDPSocket::FPCSend(PlayerRef player, SendBufferRef sendBuffer)
{
	//NetAddress netAddr = player->netAddress;
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	header->playerId = player->playerId;

	SOCKADDR_IN netaddr = player->netAddress.GetSockAddr();
	//cout << netAddr.GetPort()<< endl;
	//wcout << netAddr.GetIpAddress() << endl;
	int32 addrLen = sizeof(netaddr);
	while (true)
	{
		if (::sendto(_socket, reinterpret_cast<const char*>(sendBuffer->Buffer()), sendBuffer->WriteSize(), 0, reinterpret_cast<SOCKADDR*>(&netaddr), addrLen) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;
			break;
		}
		else break;
	}
	return sendBuffer->WriteSize();
}

