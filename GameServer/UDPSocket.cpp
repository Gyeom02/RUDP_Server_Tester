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

				// TODO  다른 방식으로 숨겨야함
				
				if (header->id != PKT_C_INIT && header->id != PKT_S_INIT)
				{
					PlayerRef player = GPlayerManager.GetPlayer(header->playerId);
					if (!player)
					{
						processLen += header->size;
						continue;
					}
					if (player->GetDeliveryManager()->CheckPacketChannel(header->channel, header->sn) == false)
					{
						processLen += header->size;
						continue;
					}
					GQoS->PushRecv(header->playerId, &udpRecvBuffer.ReadPos()[processLen], header->size);
					processLen += header->size;
				}
				else //새로 연결한 클라이언트
				{
					BYTE* cpybuffer = new BYTE[1000];
					::memcpy(cpybuffer, &udpRecvBuffer.ReadPos()[processLen], header->size);

					ClientPacketHandler::HandlePacket(shared_from_this(), NetAddress(recvAddr), cpybuffer, header->size);
					processLen += header->size;
				}
				// TODO  다른 방식으로 숨겨야함

				

				
				
				//GUDP.CheckPacketPriority(shared_from_this(), NetAddress(recvAddr), cpybuffer, header->size);

				//
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
		// FD_CLOSE 처리
		if (networkEvents.lNetworkEvents & FD_CLOSE)
		{
			// TODO : Remove Socket

		}
	}
}

int32 UDPSocket::Send(PlayerRef player, SendBufferRef sendBuffer)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	if (header->channel == QoSCore::Channel::RPCT)
	{
		if (player->CanRPCT())
		{
			FPCSend(player, sendBuffer);
			return 0;
		}
	}
	GQoS->PushSend(player->playerId, sendBuffer);
	return 0;
}


int32 UDPSocket::PriortySend(PlayerRef player, SendBufferRef sendBuffer)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	/*if (header->priority == QoSCore::FPC)
		return FPCSend(player, sendBuffer);*/
	switch (header->channel)
	{
	case QoSCore::Channel::RO:
		return ReliableSend(player, sendBuffer);
	case QoSCore::Channel::URO:
		return UnReliable_Ordered_Send(player, sendBuffer);
	case QoSCore::Channel::RPCT:
		return UnReliableSend(player, sendBuffer);
	default:
		break;
	}
	
		

}
int UDPSocket::ReliableSend(PlayerRef player, SendBufferRef sendBuffer)
{
	NetAddress netAddr = player->netAddress;
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	//header->playerId = player->playerId;

	player->GetDeliveryManager()->WriteSeqeuenceNumber(player->ownerSocket->GetSocket(), netAddr, sendBuffer);

	return FPCSend(player, sendBuffer);
}

int UDPSocket::UnReliable_Ordered_Send(PlayerRef player, SendBufferRef sendBuffer)
{
	
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	//header->playerId = player->playerId;

	player->GetDeliveryManager()->WriteSeqeuenceNumber_URO(sendBuffer);

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
	//cout << "FPCSend : " << player->playerId << endl;
	header->playerId = player->playerId;
	//cout << "FPCSend PlayerID : " << header->playerId << endl;
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
			//cout << header->playerId << " : Failed Sending PAcket" << endl;
			break;
		}
		else break;
	}
	return sendBuffer->WriteSize();
}

