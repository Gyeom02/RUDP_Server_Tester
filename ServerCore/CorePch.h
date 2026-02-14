#pragma once
#define PRVT_SERVERADDR L"127.0.0.1"
//#define _LOCKTIMEOUT_CHECK
#define CHECK_DEADLOCK

#include "Types.h"
#include "CoreMacro.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "Container.h"
#include "RUDPUtils.h"

#include <windows.h>
#include <iostream>
using namespace std;

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <chrono>

#include "Lock.h"
#include "ObjectPool.h"
#include "TypeCast.h"
#include "Memory.h"
#include "SendBuffer.h"
#include "Session.h"
#include "JobQueue.h"
#include "ConsoleLog.h"

#include "DeliveryNotificationManager.h"
#include "QoSCore.h"
#include "UDP.h"
#include "UDPRecvHandler.h"
#include "UDPSocket.h"
#include "HostManager.h"
#include "TransportControl.h"

//#include "UDP.h"