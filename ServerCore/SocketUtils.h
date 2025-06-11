#pragma once
#include "NetAddress.h"

/*----------------
	SocketUtils
-----------------*/

class SocketUtils
{
public:
	static LPFN_CONNECTEX		ConnectEx;
	static LPFN_DISCONNECTEX	DisconnectEx;
	static LPFN_ACCEPTEX		AcceptEx;

public:
	static void Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET object, GUID guid, LPVOID* fn);
	static SOCKET CreateSocket();

	static bool SetLinger(SOCKET object, uint16 onoff, uint16 linger);
	static bool SetReuseAddress(SOCKET object, bool flag);
	static bool SetRecvBufferSize(SOCKET object, int32 size);
	static bool SetSendBufferSize(SOCKET object, int32 size);
	static bool SetTcpNoDelay(SOCKET object, bool flag);
	static bool SetUpdateAcceptSocket(SOCKET object, SOCKET listenSocket);

	static bool Bind(SOCKET object, NetAddress netAddr);
	static bool BindAnyAddress(SOCKET object, uint16 port);
	static bool Listen(SOCKET object, int32 backlog = SOMAXCONN);
	static void Close(SOCKET& object);
};

template<typename T>
static inline bool SetSockOpt(SOCKET object, int32 level, int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(object, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}