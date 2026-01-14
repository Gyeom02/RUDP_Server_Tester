#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include <tchar.h>
#include "Protocol.pb.h"
#include "Job.h"
#include "Room.h"
#include "Player.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "XmlParser.h"
#include "DBSynchronizer.h"
#include "GenProcedures.h"
#include "UDP.h"
//#include "OManager.h"

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}
void PacketDeliverCondition(PlayerRef player)
{
	if (!player)
		return;
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	//GetConsoleScreenBufferInfo(h, &info);
	COORD pos = { (SHORT)0, (SHORT)player->client_Id };
	SetConsoleCursorPosition(h, pos);

	DeliveryManagerRef GDeliveryManager = player->GetDeliveryManager();
	cout << "플레이어 ID : " << player->client_Id << " 전체 보낸 패킷 수 : " << GDeliveryManager->GetDispatchedPacketCount() << " 성공패킷 : " << GDeliveryManager->GetDeliveredPacketCount()
		<< " 실패패킷 : " << GDeliveryManager->GetDroppedPacketCount() - GDeliveryManager->GetSuccessReSendPacketNum() << " 성공 + 실패 : " << GDeliveryManager->GetDeliveredPacketCount() + (GDeliveryManager->GetDroppedPacketCount() - GDeliveryManager->GetSuccessReSendPacketNum()) << endl;
}


int main()
{
	//ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={SQL Server Native Client 11.0};Server=(localdb)\\MSSQLLocalDB;Database=ServerDb;Trusted_Connection=Yes;"));
	/*
	{
		auto query = L"         \
			DROP TABLE IF EXISTS [dbo].[Gold];\
			CREATE TABLE [dbo].[Gold] \
			(\
				[id] BIGINT NOT NULL PRIMARY KEY IDENTITY, \
				[gold] BIGINT NULL, \
				[name] NVARCHAR(50) NULL, \
				[createDate] DATETIME NULL \
			);";
		
		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
	}*/

	//DBConnection* dbConn = GDBConnectionPool->Pop();
	//DBSynchronizer dbSync(*dbConn);
	//dbSync.Synchronize(L"GameDB.xml");
	// 
	//
	//{
	//	SP::RemoveGold removeGold(*dbConn);
	//	removeGold.Execute();
	//}

	//{
	//	WCHAR name[] = L"Rookiss";

	//	SP::InsertGold insertGold(*dbConn);
	//	insertGold.In_Gold(100);
	//	insertGold.In_Name(name);
	//	insertGold.In_CreateDate(TIMESTAMP_STRUCT{ 2025, 1, 22 });
	//	insertGold.Execute();
	//}

	//{
	//	SP::GetGold getGold(*dbConn);
	//	getGold.In_Gold(100);

	//    int32 id = 0;
	//	int32 gold = 0;
	//	WCHAR name[100];
	//	TIMESTAMP_STRUCT date;

	//	getGold.Out_Id(OUT id);
	//	getGold.Out_Gold(OUT gold);
	//	getGold.Out_Name(OUT name);
	//	getGold.Out_CreateDate(OUT date);

	//	getGold.Execute();

	//	while (getGold.Fetch())
	//	{
	//		GConsoleLogger->WriteStdOut(Color::BLUE,
	//			L"ID[%d] Gold[%d] Name[%s] CreateDate [%d] , [%d], [%d]\n", id, gold, name, date.year, date.month, date.day);
	//	}
	//}

	ClientPacketHandler::Init(); 
	
	//ServerServiceRef service = MakeShared<ServerService>(
	//	NetAddress(L"127.0.0.1", 7777),
	//	MakeShared<IocpCore>(),
	//	MakeShared<GameSession>, // TODO : SessionManager 등
	//	100);

	//ASSERT_CRASH(service->Start());

	//for (int32 i = 0; i < 5; i++)
	//{
	//	GThreadManager->Launch([&service]()
	//		{
	//			DoWorkerJob(service);
	//		});
	//}

	//// Main Thread
	//DoWorkerJob(service);
	if (GUDP.UDPInit(UDP::SERVER))
	{
		cout << "UDP Init Succeed" << endl;
		GUDP.SetIsOn(true);
	}
	if (GUDP.IsUDPOn())
	{
		for (int32 i = 0; i < UDP::SOCKNUM; i++)
		{
			GThreadManager->Launch([=]()
				{
					GUDP.GetUDPSocket(i)->UDPWork();
				});

		}
		/*for (int32 i = 0; i < 2; i++)
		{
			GThreadManager->Launch([=]()
				{
					GUDP.UDPDoWork();
				});

		}*/
		//Thread
		GUDP.InitRecvLogicWorkers(&ClientPacketHandler::HandlePacket);
	}

	GTransportControl.RunThread();

	//uint32 ackpreStart = 0;
	//uint32 ackStart = 1;
	//uint32 ackCount = 0;
	//bool hasCount = false;
	//NetAddress netAddr;


	////system("cls");

	////Thread 최적화 필요
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
	//			    //cout << "Send RUDP ACK  " << endl;
	//			}
	//			else
	//				break;
	//		}
	//		p.second->GetDeliveryManager()->ProcessTimeOutPackets();
	//		//PacketDeliverCondition(static_pointer_cast<Player>(p.second));
	//	}
	//}
	GThreadManager->Join();
	return 0;
}