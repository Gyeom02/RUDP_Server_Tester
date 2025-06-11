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
#include "PlayerManager.h"

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
	DeliveryManagerRef GDeliveryManager = player->GetDeliveyManager();
	cout << "플레이어 ID : " << player->playerId << " 전체 보낸 패킷 수 : " << GDeliveryManager->GetDispatchedPacketCount() << " 성공패킷 : " << GDeliveryManager->GetDeliveredPacketCount()
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
	if (GUDP.UDPInit())
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
		for (int32 i = 0; i < 2; i++)
		{
			GThreadManager->Launch([=]()
				{
					GUDP.UDPDoJop();
				});

		}
		
	}
	int32 ackpreStart = 0;
	int32 ackStart = 1;
	int32 ackCount = 0;
	bool hasCount = false;
	NetAddress netAddr;
	while (true)
	{
		if (GPlayerManager.GetPlayers().empty())
			continue;
		for (auto p : GPlayerManager.GetPlayers())
		{
			//auto player = p.second;
			memset(&netAddr, 0, sizeof(netAddr));
			//this_thread::sleep_for(300ms);
			while (true) //AckRange 비울때까지
			{

				if (p.second->GetDeliveyManager()->WritePendingAcks(ackStart, ackCount, hasCount)) // 보낼 Ack이 쌓였다
				{
					/*if (ackpreStart == ackStart && ackpreStart > 1)
					{
						cout << "ackpreStart : " << ackpreStart << endl;
						CRASH("ackpreStart == ackStart");
					}*/
					Protocol::S_RUDPACK pkt;
					pkt.set_bhascount(hasCount);
					pkt.set_count(ackCount);
					pkt.set_start(ackStart);
					pkt.set_playerid(p.second->playerId);
					SendBufferRef sendBuffer = ClientPacketHandler::MakeUnReliableBuffer(pkt);
					p.second->Send(sendBuffer);
					//GUDP.GetUDPSocket(0)->Send(netAddr, sendBuffer);
					ackpreStart = ackStart;
					//cout << "Send RUDP ACK player->playerId : " << p.second->playerId << endl;
				}
				else
					break;
			}
			p.second->GetDeliveyManager()->ProcessTimeOutPackets();
			//PacketDeliverCondition();
		}
	}
	GThreadManager->Join();
	return 0;
}