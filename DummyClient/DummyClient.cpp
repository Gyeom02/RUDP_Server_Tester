#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "Player.h"
char sendData[] = "Hello World";
#define SERVERADDR L"192.168.219.147"


class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		Protocol::C_LOGIN pkt;
		auto sendBuffer = ServerPacketHandler::MakeReliableBuffer(pkt, QoSCore::LOW);
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// TODO : packetId 대역 체크
	//	ServerPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		//cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		//cout << "Disconnected" << endl;
	}
};


int32 Send(int32 id, UDPSocketPtr udpSocket, NetAddress netAddr, SendBufferRef sendBuffer)
{
	PlayerRef player = static_pointer_cast<Player>(GHostManager.GetPlayer(id));
	player->Send(sendBuffer);
	//GPlayerManager._sendPacketNum++;
	return sendBuffer->WriteSize();
}

int32 Init_Send()
{
	SOCKADDR_IN netaddr = GUDP.GetUDPSocket(0)->GetNetAddress().GetSockAddr();
	UDPSocketPtr socketPtr = GUDP.GetUDPSocket(0);
	Protocol::C_INIT pkt;
	SendBufferRef sendBuffer = ServerPacketHandler::MakeUnReliableBuffer(pkt);
	//cout << netAddr.GetPort()<< endl;
	//wcout << netAddr.GetIpAddress() << endl;
	int32 addrLen = sizeof(netaddr);
	while (true)
	{
		if (::sendto(socketPtr->GetSocket(), reinterpret_cast<const char*>(sendBuffer->Buffer()), sendBuffer->WriteSize(), 0, reinterpret_cast<SOCKADDR*>(&netaddr), addrLen) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;
			cout << "Send Faild : " << ::WSAGetLastError() << endl;
			break;
		}
		else
		{
			cout << "Send Success : " << sendBuffer->WriteSize() << endl;
			break;
		}
	}
	return sendBuffer->WriteSize();
}

int32 Recv(UDPSocketPtr udpSocket, NetAddress netAddr)
{
	RecvBuffer recvBuffer(65536);
	PacketHeader* header = reinterpret_cast<PacketHeader*>(recvBuffer.WritePos());
	
	
	SOCKADDR_IN netaddr;
	memset(&netaddr, 0, sizeof(SOCKADDR_IN));
	//cout << netAddr.GetPort()<< endl;
	//wcout << netAddr.GetIpAddress() << endl;
	int32 addrLen = sizeof(netaddr);
	int32 Len;
	while (true)
	{
		Len = ::recvfrom(udpSocket->GetSocket(), reinterpret_cast<char*>(recvBuffer.WritePos()), recvBuffer.FreeSize(), 0, reinterpret_cast<SOCKADDR*>(&netaddr), &addrLen);
		if (Len == SOCKET_ERROR)
		{
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
			{
				continue;
			}
				
			cout << "Recv Faild : " << ::WSAGetLastError() << endl;
			break;
		}
		else {
			cout << "Recv Success : " << Len << endl;
			break;
		}
	}
	//cout << "Recved" << endl;
	ServerPacketHandler::HandlePacket(udpSocket, netAddr, recvBuffer.WritePos(), Len);
	return recvBuffer.DataSize();
}

void PacketDeliverCondition(PlayerRef player)
{
	//cout << "GetSuccessReSendPacketNum : " << GDeliveryManager->GetSuccessReSendPacketNum() << endl;
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	//GetConsoleScreenBufferInfo(h, &info);
	COORD pos = { (SHORT)0, (SHORT)player->client_Id };
	SetConsoleCursorPosition(h, pos);

	DeliveryManagerRef GDeliveryManager = player->GetDeliveryManager();
	cout << "플레이어 ID : " << player->client_Id << " 전체 보낸 패킷 수 : " << GDeliveryManager->GetDispatchedPacketCount() << " 성공패킷 : " << GDeliveryManager->GetDeliveredPacketCount()
	<< " 실패패킷 : " << GDeliveryManager->GetDroppedPacketCount() - GDeliveryManager->GetSuccessReSendPacketNum() << " 성공 + 실패 : " << GDeliveryManager->GetDeliveredPacketCount()+ (GDeliveryManager->GetDroppedPacketCount() - GDeliveryManager->GetSuccessReSendPacketNum()) << endl;
	//cout << " 실패패킷 : " << GDeliveryManager->GetDroppedPacketCount() << " | 다시보낸 패킷 : " << GDeliveryManager->GetSuccessReSendPacketNum() << endl;
}

void PacketLost(PlayerRef player)
{
	cout << " Not Matched : " << player->GetDeliveryManager()->GetSequenceNotMatchedCount() << " | TimeOut : " << player->GetDeliveryManager()->GetTimeOutCount() << endl;
}

void CloseApp()
{
	for (auto& [id, object] : GHostManager.GetPlayers())
	{
		shared_ptr<Player> player = static_pointer_cast<Player>(object);
		Protocol::C_DISCONNECT pkt;
		pkt.set_id(player->client_Id);
		pkt.set_roomid(player->roomId);
		pkt.set_roomprimid(player->roomprimid);
		SendBufferRef sendBuffer = ServerPacketHandler::MakeReliableBuffer(pkt, QoSCore::HIGH);

		player->Send(sendBuffer);
	}
}
BOOL WINAPI ConsoleHandler(DWORD signal) {
	switch (signal) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		std::cout << "[ConsoleHandler] 종료 신호 수신됨." << std::endl;
		
		CloseApp();  // 종료 전 func 호출

		Sleep(500);
		return TRUE;
	default:
		return FALSE;
	}
}
int main()
{
	if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
		cout << "SetConsoleCtrlHandler Failed" << endl;
	else
		cout << "SetConsoleCtrlHandler Succeed" << endl;
	this_thread::sleep_for(1s);

	if (GUDP.UDPInit(UDP::CLIENT))
	{
		cout << "UDP Init Succeed" << endl;
		GUDP.SetIsOn(true);
	}
	ServerPacketHandler::Init();

	for (int32 i = 0; i < GUDP.SOCKNUM; i++)
	{
		
		Init_Send();
		 
		Recv(GUDP.GetUDPSocket(0), GUDP.GetUDPSocket(0)->GetNetAddress());
	}
	this_thread::sleep_for(1s);

	//ClientServiceRef service = MakeShared<ClientService>(
	//	NetAddress(L"127.0.0.1", 7777),
	//	MakeShared<IocpCore>(),
	//	MakeShared<ServerSession>, // TODO : SessionManager 등
	//	500);

	//ASSERT_CRASH(service->Start());

	//for (int32 i = 0; i < 2; i++)
	//{
	//	GThreadManager->Launch([=]()
	//		{
	//			while (true)F
	//			{
	//				service->GetIocpCore()->Dispatch();
	//			}
	//		});
	//}
	//SOCKET socket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	//if (socket == INVALID_SOCKET)
	//{
	//	cout << "Client INVALID SOCKET " << endl;
	//	return 0;
	//}
	/*u_long flag = 1;
	if (::ioctlsocket(socket, FIONBIO, &flag) == INVALID_SOCKET)
	{
		cout << "Failed Non-Blocking UDP Socket" << endl;
	}*/
	//SOCKADDR_IN serverAddress;
	//serverAddress.sin_family = AF_INET;
	//serverAddress.sin_port = 7777;

	//IN_ADDR addr;
	//memset(&addr, 0, sizeof(SOCKADDR));
	//InetPtonW(AF_INET, SERVERADDR, &addr);
	//serverAddress.sin_addr = addr;

	//NetAddress netAddress(serverAddress);

	
	//while(true)	
	//	::sendto(socket, reinterpret_cast<const char*>(sendBuffer->Buffer()), sendBuffer->WriteSize(), 0, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
	/*while (true)
	{
		service->Broadcast(sendBuffer);
		this_thread::sleep_for(1s);
	}*/
	ULONGLONG timeout = 15000;
	ULONGLONG now = ::GetTickCount64();
	//GThreadManager->Launch([=]() {
	//	while (true)
	//	{
	//		for (auto& p : GPlayerManager.GetPlayers())
	//		{
	//			PlayerRef player = p.second;
	//			if (player && player->playerId != 0)
	//				player->ProcessTimeOutPackets();
	//		}
	//		//if (GetTickCount64() - now >= timeout)
	//			//break;
	//	}
	//	});
	for (int32 i = 0; i < GUDP.SOCKNUM; i++) //Socket 마다 RecvFrom Work Thread를 생성하는거임 (비효율적임 나중에 개선)
		GThreadManager->Launch([=]() {
			while (true)
			{
				//Recv(static_pointer_cast<UDPSocket>(_player->GetOwnerSocket()), _player->GetNetAddr());
			
				GUDP.GetUDPSocket(i)->UDPWork();
				//if (GetTickCount64() - now >= timeout + 5000)
					//break;
			}
			});
	//for (int32 i = 0; i < 3; i++) //Recv된 패킷을 처리하는 Thread들을 생성하는것
	//{
	//	GThreadManager->Launch([=]()
	//		{
	//			GUDP.UDPDoWork();

	//		});

	//}

	GUDP.InitRecvLogicWorkers(&ServerPacketHandler::HandlePacket); //Recv된 패킷을 처리하는 Thread들을 생성하는것

	/*Protocol::C_INIT chatPktt;
	auto sendBufferr = ServerPacketHandler::MakeSendBuffer(chatPktt);
	for(auto& p : GPlayerManager.GetPlayers())
		Send(p.second->playerId, static_pointer_cast<UDPSocket>(p.second->GetOwnerSocket()), p.second->GetNetAddr(), sendBufferr);
	*/

	string longtext = "";
	while (longtext.size() < 3000)
		longtext += "ABCDEFGHIJKLNMOPQRSWZXABCDEFGHIJKLNMOPQRSWZXABCDEFGHIJKLNMOPQRSWZX";
	this_thread::sleep_for(1s);
	for (int32 i = 0; i < 1; i++) // i = 패킷 강도를 나타냄
		GThreadManager->Launch([=]() // 플레이어 클래스마다 RUDP AckRange 클래스 배열을 갖고있고 돌아가면서 차있으면 AckRange 정보를 송신한다
			{

				while (true)
				{
					this_thread::sleep_for(16ms);
					if (GHostManager.GetPlayers().empty())
						continue;
					for (auto& p : GHostManager.GetPlayers())
					{
						if (p.second->client_Id != 0)
						{
							Protocol::C_MSG chatPktt;
							chatPktt.set_msg("Hello Server");
							//auto sendBufferchatPkttt = ServerPacketHandler::MakeReliableBuffer(chatPktt, QoSCore::LOW);
							//Send(p.second->client_Id, static_pointer_cast<UDPSocket>(p.second->ownerSocket), p.second->netAddress, sendBufferchatPkttt);
							//cout << "SENDING MSG ID : " << p.second->playerId << endl;
							Protocol::C_MSG chatPkt;
							chatPkt.set_msg(longtext);
							
							auto sendBufferchatPkt = ServerPacketHandler::MakeReliableBuffer(chatPkt, QoSCore::LOW);
							Send(p.second->client_Id, p.second->ownerSocket, p.second->netAddress, sendBufferchatPkt);

							/*auto sendBufferchatPktt = ServerPacketHandler::MakeUnReliableBuffer(chatPktt);
							Send(p.second->playerId, static_pointer_cast<UDPSocket>(p.second->ownerSocket), p.second->netAddress, sendBufferchatPktt);
							*/
							
						//	auto sendBufferchatPktttt = ServerPacketHandler::MakeReplicateBuffer(chatPktt);
						//	Send(p.second->client_Id, static_pointer_cast<UDPSocket>(p.second->ownerSocket), p.second->netAddress, sendBufferchatPktttt);
						}
					}
					//this_thread::sleep_for(200ms);
					//PacketDeliverCondition();

					//if (GetTickCount64() - now >= timeout)
						//break;
				}
				
			});

	//	cout << "a" << endl;
	//while(true)
	//	GUDP.UDPDoJop();
	uint32 start =0;
	uint32 count = 0;
	bool bhascount;
	
	system("cls");

	//while (true)
	//{
	//	//this_thread::sleep_for(100ms);
	//	if (GHostManager.GetPlayers().empty())
	//		continue;
	//	for (auto& p : GHostManager.GetPlayers())
	//	{
	//		PlayerRef player = static_pointer_cast<Player>(p.second);
	//		if (player && player->client_Id != 0)
	//		{
	//			//this_thread::sleep_for(100ms);
	//			while (true) //AckRange 비울때까지
	//			{
	//				if (player->GetDeliveryManager()->WritePendingAcks(start, count, bhascount)) // 보낼 Ack이 쌓였다
	//				{
	//					Protocol::C_RUDPACK pkt;
	//					pkt.set_bhascount(bhascount);
	//					pkt.set_count(count);
	//					pkt.set_start(start);
	//					pkt.set_playerid(player->client_Id);
	//					pkt.set_rwindsize(player->GetRWind());
	//					SendBufferRef sendBufferR = ServerPacketHandler::MakeUnReliableBuffer(pkt);
	//					player->Send(sendBufferR);
	//					//cout << "Send RUDP ACK" << endl;
	//				}
	//				else
	//					break;
	//			}
	//		}
	//		player->GetDeliveryManager()->ProcessTimeOutPackets();
	//		//PacketLost();
	//	//	PacketDeliverCondition(player);
	//	}
	//	
	//}
	GTransportControl.RunThread();

	//while (true)
	//{
	//	for (auto& p : GHostManager.GetPlayers())
	//	{	//	{
	//		//		PlayerRef player = static_pointer_cast<Player>(p.second);
	//		PlayerRef player = static_pointer_cast<Player>(p.second);
	//		if (player && player->client_Id != 0)
	//		{
	//			PacketDeliverCondition(player);
	//		}
	//	}
	//}
	//uint32 ackpreStart = 0;
	//uint32 ackStart = 1;
	//uint32 ackCount = 0;
	//bool hasCount = false;
	//NetAddress netAddr;

	//while (true)
	//{
	//	for (auto& p : GHostManager.GetPlayers())
	//	{
	//		//auto player = p.second;
	//		memset(&netAddr, 0, sizeof(netAddr));
	//		//this_thread::sleep_for(300ms);
	//	   // int32 token = 5;
	//		while (true) //AckRange 비울때까지
	//		{
	//			//token--;

	//			if (p.second->GetDeliveryManager()->WritePendingAcks(ackStart, ackCount, hasCount)) // 보낼 Ack이 쌓였다
	//			{
	//				/*if (ackpreStart == ackStart && ackpreStart > 1)
	//				{
	//					cout << "ackpreStart : " << ackpreStart << endl;
	//					CRASH("ackpreStart == ackStart");
	//				}*/
	//				SendBufferRef sendBuffer = TransportControlPlane::MakeControlPacketBuffer(p.second->client_Id);
	//				ControlHeader* contheader = reinterpret_cast<ControlHeader*>(sendBuffer->Buffer() + sizeof(PacketHeader));
	//				//  Protocol::S_RUDPACK pkt;
	//				contheader->ack.bexsist = true;
	//				contheader->ack.bhascount = hasCount;
	//				contheader->ack.count = ackCount;
	//				contheader->ack.start = ackStart;

	//				// pkt.set_playerid(p.second->client_Id);
	//				//contheader->rwind.bexsist = true;
	//				//contheader->rwind.rwindsize = p.second->GetRWind();
	//				// pkt.set_rwindsize();

	//				p.second->NoWaitPriortySend(sendBuffer);
	//				//GUDP.GetUDPSocket(0)->Send(netAddr, sendBuffer);
	//			  //  ackpreStart = ackStart;
	//			   //  cout << "Send RUDP ACK  " << endl;
	//			}
	//			else
	//				break;
	//		}
	//		p.second->GetDeliveryManager()->ProcessTimeOutPackets();
	//		PacketDeliverCondition(static_pointer_cast<Player>(p.second));
	//	}
	//	
	//}
	GThreadManager->Join();
	
	return 0;
}
