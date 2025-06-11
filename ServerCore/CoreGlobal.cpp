#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "Memory.h"
#include "DeadLockProfiler.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"
#include "ConsoleLog.h"
#include "QoSCore.h"
//#include "DeliveryNotificationManager.h"

ThreadManager*		GThreadManager = nullptr;
Memory*				GMemory = nullptr;
SendBufferManager*	GSendBufferManager = nullptr;
GlobalQueue*		GGlobalQueue = nullptr;
JobTimer*			GJobTimer = nullptr;
//DeliveryNotificationManager* GDeliveryManager = nullptr;

DeadLockProfiler*	GDeadLockProfiler = nullptr;
DBConnectionPool*	GDBConnectionPool = nullptr;
ConsoleLog*			GConsoleLogger = nullptr;
QoSCore* GQoS = nullptr;
class CoreGlobal
{
public:
	CoreGlobal()
	{
		
		GThreadManager = new ThreadManager();
		GMemory = new Memory();
		GSendBufferManager = new SendBufferManager();
		//GDeliveryManager = new DeliveryNotificationManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();
		GDeadLockProfiler = new DeadLockProfiler();
		GDBConnectionPool = new DBConnectionPool();
		GConsoleLogger = new ConsoleLog();
		GQoS = new QoSCore();;
		SocketUtils::Init();

	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GMemory;
		delete GSendBufferManager;
		//delete GDeliveryManager;
		delete GGlobalQueue;
		delete GJobTimer;
		delete GDeadLockProfiler;
		delete GDBConnectionPool;
		delete GConsoleLogger;
		delete GQoS;
		SocketUtils::Clear();
	//	delete GUDP;
	}
} GCoreGlobal;