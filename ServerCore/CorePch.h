#pragma once
#define SERVERADDR L"192.168.219.105"
#define _LOCKTIMEOUT_CHECK

#include "Types.h"
#include "CoreMacro.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "Container.h"

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
#include "TransportControlPlane.h"

//#include "UDP.h"