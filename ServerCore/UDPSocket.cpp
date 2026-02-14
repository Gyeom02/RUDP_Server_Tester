#include "pch.h"
#include "UDPSocket.h"
#include "UDP.h"
//#include "HostManager.h"
#include "QoSCore.h"
#include "RTTManager.h"

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
		//cout << "Recv" << endl;
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
				//cout << "Recv Error : " << ::WSAGetLastError() << endl;
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
				
				if (header->id != 1002 && header->id != 1003) // PKT_C_INIT, PKT_S_INIT 임시방편
				{
					HostRef player = GHostManager.GetPlayer(header->client_Id);
					if (!player)
					{
						//cout << "if (!player)" << endl;
						//CRASH("!player");
						processLen += header->size;
						continue;
					}
					/*if(header->channel == QoS::Channel::RO)
						if(header->controlflag != 0)
							cout << "header->channel == QoSCore::Channel::RO | SN : " << header->sn << " | header->ControlFlag : " << header->controlflag << endl;
					*/
					
					
					if (GTransportControl.CheckValidControl(header)) // ControlPacket이다
					{
						//cout << "CheckValidControl SN : " << header->sn << endl;
					}
					else
					{
						if (player->GetDeliveryManager()->CheckPacketChannel(header) == false)
						{
							//cout << "(player->GetDeliveryManager()->CheckPacketChannel(header->channel, header->sn) == false) SN : " << header->sn<< endl;
							processLen += header->size;
							continue;
						}
						//std::cout << "Handle_OnRecv | Client ID : " << header->client_Id << endl;
						GQoS->OnRecv(player->GetRWindExpectedSeqNum(), header->client_Id, &udpRecvBuffer.ReadPos()[processLen], header->size);
					}
					processLen += header->size;
				}
				else //새로 연결한 클라이언트
				{
					//cout << "1002 1003" << endl;
					BYTE* cpybuffer = new BYTE[1000];
					::memcpy(cpybuffer, &udpRecvBuffer.ReadPos()[processLen], header->size);

					GUDP._func(shared_from_this(), NetAddress(recvAddr), cpybuffer, header->size);
					//ClientPacketHandler::HandlePacket();
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

int32 UDPSocket::Send(HostRef player, SendBufferRef sendBuffer)
{
	if (CheckMSSover(sendBuffer))
		SizeOverSend(player, sendBuffer);
	else
	{
		shared_ptr<vector<SendBufferRef>> buffers = MakeShared<vector<SendBufferRef>>();
		buffers->push_back(sendBuffer);
		NormalSend(player, buffers);
	}
	return 0;
}

int32 UDPSocket::PriortySend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffers)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>((*sendBuffers)[0]->Buffer());
	/*if (header->priority == QoSCore::FPC)
		return FPCSend(player, sendBuffer);*/
	switch (header->channel)
	{
	case QoS::Channel::RO:
		return ReliableSend(player, sendBuffers);
	case QoS::Channel::URO:
		return UnReliable_Ordered_Send(player, (*sendBuffers)[0]); // URO must not be fragment packet
	case QoS::Channel::RPCT:
		return UnReliableSend(player, (*sendBuffers)[0]); // RFCT must not be fragment packet
	default:
		break;
	}



}

int32 UDPSocket::ControlSend(HostRef player, SendBufferRef sendBuffer)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	/*if (header->priority == QoSCore::FPC)
		return FPCSend(player, sendBuffer);*/
	/*if(header->channel == QoS::Channel::URO)
	{

		return UnReliable_Ordered_Send(player, sendBuffer);

	}*/
	UnReliableSend(player, sendBuffer);

	return -1;

}
bool UDPSocket::CheckMSSover(SendBufferRef sendBuffer)
{
	if (sendBuffer->WriteSize() > USER_MSS)
		return true;
	return false;
}
//
//int32 UDPSocket::NormalSend(HostRef player, SendBufferRef sendBuffer)
//{
//	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
//	if (header->channel == QoSCore::Channel::RPCT)
//	{
//		if (player->CanRPCT())
//		{
//			FPCSend(player, sendBuffer);
//			return 0;
//		}
//	}
//	GQoS->PushSend(player->client_Id, sendBuffer);
//	return 0;
//}
int32 UDPSocket::NormalSend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffer)
{
	if ((*sendBuffer).empty())
		return 0;
	PacketHeader* header = reinterpret_cast<PacketHeader*>((*sendBuffer)[0]->Buffer());
	if (header->channel == QoS::Channel::RPCT)
	{
		if (player->CanRPCT())
		{
			FPCSend(player, sendBuffer);
			return 0;
		}
	}
	GQoS->PushSend(player->client_Id, sendBuffer);
	return 0;
}
int32 UDPSocket::SizeOverSend(HostRef player, SendBufferRef sendBuffer)
{
	const PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());

	int32 payloadSize = header->size - sizeof(PacketHeader); // 게산에 사용됨
	const int32 const_payloadSize = payloadSize; // 원본 Payload를 보여줌
	const int32 divideNum = payloadSize / (USER_MSS - sizeof(FragmentHeader)) + 1; // 조각화 수를 보여줌
	int32 dividePacketBaseSize = (payloadSize / divideNum) + (divideNum - 1);
	
	int32 packetDivideIndex = 0; // 조각화된 순서
	int32 Finished_Size = 0; //패킷 조각화 처리가 끝난 전체 크기(이를 통해 다음에 진행할 조각화 패킷의 offset을 구할 수 있음)
	
	shared_ptr<vector<SendBufferRef>> _bufferRef = MakeShared<vector<SendBufferRef>>();
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
		packetDivideIndex = i;
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


		memcpy(fragmentSendBuffer->Buffer() + sizeof(PacketHeader) + sizeof(FragmentHeader), sendBuffer->Buffer() + sizeof(PacketHeader) + Finished_Size, newPayloadSize);
		

		_bufferRef->push_back(fragmentSendBuffer);

		Finished_Size += newPayloadSize;
		packetDivideIndex++;
		payloadSize -= newPayloadSize;
	}

	NormalSend(player, _bufferRef);
	return 0;
}

SendBufferRef UDPSocket::MakeFragmentBuffer(int32 size)
{
	SendBufferRef fragmentSendBuffer = GSendBufferManager->Open(size);
	fragmentSendBuffer->Close(size);

	return fragmentSendBuffer;
}

//int UDPSocket::ReliableSend(HostRef player, SendBufferRef sendBuffer)
//{
//	NetAddress netAddr = player->netAddress;
//	//PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
//	//header->playerId = player->playerId;
//
//	player->GetDeliveryManager()->WriteSeqeuenceNumber(sendBuffer);
//
//	return FPCSend(player, sendBuffer);
//}

//int UDPSocket::UnReliable_Ordered_Send(HostRef player, SendBufferRef sendBuffer)
//{
//	player->GetDeliveryManager()->WriteSeqeuenceNumber_URO(sendBuffer);
//
//	return FPCSend(player, sendBuffer);
//}

//int UDPSocket::UnReliableSend(HostRef player, SendBufferRef sendBuffer)
//{
//	return FPCSend(player, sendBuffer);
//}

int UDPSocket::FPCSend(HostRef player, SendBufferRef sendBuffer)
{

	//int32 sendLen = 0;
	
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	//cout << "FPCSend : " << player->playerId << endl;
	header->client_Id = player->client_Id;
	//sendLen += sendBuffer->WriteSize();
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



int UDPSocket::ReliableSend(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffers)
{
	NetAddress netAddr = player->netAddress;
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	//header->playerId = player->playerId;

	player->GetDeliveryManager()->WriteSeqeuenceNumber(sendBuffers); //Move To QoSCore.cpp QoSPlayer::PopSend


	return FPCSend_RTT(player, sendBuffers);
}

int UDPSocket::UnReliable_Ordered_Send(HostRef player, SendBufferRef sendBuffer)
{
	
	//PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	//header->playerId = player->playerId;

	player->GetDeliveryManager()->WriteSeqeuenceNumber_URO(sendBuffer);
	
	
	return FPCSend(player, sendBuffer);
}


int UDPSocket::UnReliableSend(HostRef player, SendBufferRef sendBuffer)
{
	return FPCSend(player, sendBuffer);
}


int UDPSocket::FPCSend(HostRef player, shared_ptr<vector<SendBufferRef>>sendBuffer, RTTFunc func)
{
	//NetAddress netAddr = player->netAddress;
	PacketHeader* header;
	SOCKADDR_IN netaddr = player->netAddress.GetSockAddr();
	//cout << netAddr.GetPort()<< endl;
	//wcout << netAddr.GetIpAddress() << endl;
	int32 addrLen = sizeof(netaddr);
	int32 sendLen = 0;
	for(int32 i = 0; i < (*sendBuffer).size(); i++)
	{ 
		header = reinterpret_cast<PacketHeader*>((*sendBuffer)[i]->Buffer());
	  //  cout << "FPCSend Header SN : " << header->sn << endl;
		header->client_Id = player->client_Id;
		sendLen += (*sendBuffer)[i]->WriteSize();
	//cout << "FPCSend PlayerID : " << header->playerId << endl;
		if (func != nullptr)
			func(header);
		WRITE_LOCK;
		while (true)
		{
			if (::sendto(_socket, reinterpret_cast<const char*>((*sendBuffer)[i]->Buffer()), (*sendBuffer)[i]->WriteSize(), 0, reinterpret_cast<SOCKADDR*>(&netaddr), addrLen) == SOCKET_ERROR)
			{
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;
				//cout << header->playerId << " : Failed Sending PAcket" << endl;
				break;
			}
			else break;
		}
		
	}
	return sendLen;
}

int UDPSocket::FPCSend_RTT(HostRef player, shared_ptr<vector<SendBufferRef>> sendBuffers)
{
	return FPCSend(player, sendBuffers, [](PacketHeader* header) { header->sent_timestamp = UTime::GetNow(); });
}

