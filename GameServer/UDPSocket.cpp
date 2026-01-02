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
						//cout << "if (!player)" << endl;
						processLen += header->size;
						continue;
					}
					if (player->GetDeliveryManager()->CheckPacketChannel(header->channel, header->sn) == false)
					{
						//cout << "(player->GetDeliveryManager()->CheckPacketChannel(header->channel, header->sn) == false)" << endl;
						processLen += header->size;
						continue;
					}
					GQoS->OnRecv(player->GetExpectedSeqNum(), header->playerId, &udpRecvBuffer.ReadPos()[processLen], header->size);
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
	if (CheckMSSover(sendBuffer))
		SizeOverSend(player, sendBuffer);
	else
		NormalSend(player, sendBuffer);
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
bool UDPSocket::CheckMSSover(SendBufferRef sendBuffer)
{
	if (sendBuffer->WriteSize() > USER_MSS)
		return true;
	return false;
}

int32 UDPSocket::NormalSend(PlayerRef player, SendBufferRef sendBuffer)
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
int32 UDPSocket::SizeOverSend(PlayerRef player, SendBufferRef sendBuffer)
{
	const PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());

	int32 payloadSize = header->size - sizeof(PacketHeader); // 게산에 사용됨
	const int32 const_payloadSize = payloadSize; // 원본 Payload를 보여줌
	const int32 divideNum = payloadSize / (USER_MSS - sizeof(FragmentHeader)) + 1; // 조각화 수를 보여줌
	int32 dividePacketBaseSize = (payloadSize / divideNum) + (divideNum - 1);
	
	int32 packetDivideIndex = 0; // 조각화된 순서
	int32 Finished_Size = 0; //패킷 조각화 처리가 끝난 전체 크기(이를 통해 다음에 진행할 조각화 패킷의 offset을 구할 수 있음)
	

	/*PacketHeader newHeader;
	memcpy(&newHeader, header, sizeof(PacketHeader));

	newHeader.bFragment = 1;
	*/

	FragmentHeader newFgHeader;
	//값이 변하지 않는 고유 값은 미리 정의
	newFgHeader.primID = player->GetFragmentPrimID();
	newFgHeader.original_size = const_payloadSize;
	newFgHeader.frag_count = divideNum;

	for (int i = 0; i < divideNum; i++)
	{
		/* 초기화 및 기본값 세팅*/

		int32 newPayloadSize = dividePacketBaseSize;
		if (dividePacketBaseSize > payloadSize)
			newPayloadSize = payloadSize;

		int32 PktTotallSize = sizeof(PacketHeader) + sizeof(FragmentHeader) + newPayloadSize;
		//PacketHeader 값 바꿈
		//newHeader.size = sizeof(PacketHeader) + sizeof(FragmentHeader) + PktSize;
		
		//FragmentHeader 값이 변하는 변수들
		newFgHeader.size = newPayloadSize;
		newFgHeader.index = packetDivideIndex;
		newFgHeader.offset = Finished_Size;
		
		/*-----------------------*/

		SendBufferRef fragmentSendBuffer = MakeFragmentBuffer(PktTotallSize);
		
		PacketHeader* newHeader = reinterpret_cast<PacketHeader*>(fragmentSendBuffer->Buffer());
		memcpy(newHeader, header, sizeof(PacketHeader));

		newHeader->bFragment = 1;
		newHeader->size = PktTotallSize;

		FragmentHeader* newFgHeader_dummy = reinterpret_cast<FragmentHeader*>(fragmentSendBuffer->Buffer() + sizeof(PacketHeader));
		memcpy(newFgHeader_dummy, &newFgHeader, sizeof(FragmentHeader));


		memcpy(fragmentSendBuffer->Buffer() + sizeof(PacketHeader) + sizeof(FragmentHeader), sendBuffer->Buffer() + sizeof(PacketHeader), newPayloadSize);
		

		NormalSend(player, fragmentSendBuffer);

		Finished_Size += newPayloadSize;
		packetDivideIndex++;
		payloadSize -= newPayloadSize;
	}
	return 0;
}

SendBufferRef UDPSocket::MakeFragmentBuffer(int32 size)
{
	SendBufferRef fragmentSendBuffer = GSendBufferManager->Open(size);
	fragmentSendBuffer->Close(size);

	return fragmentSendBuffer;
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

