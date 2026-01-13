#include "pch.h"
#include "Host.h"

void Host::Send(SendBufferRef sendBuffer)
{
	//WRITE_LOCK;

	ownerSocket->Send(static_pointer_cast<Host>(shared_from_this()), sendBuffer);
}

void Host::PriortySend(shared_ptr<vector<SendBufferRef>> sendBuffer)
{
	//WRITE_LOCK;
	ownerSocket->PriortySend(static_pointer_cast<Host>(shared_from_this()), sendBuffer);
}
void Host::NoWaitPriortySend(SendBufferRef sendBuffer)
{
	//WRITE_LOCK;
	ownerSocket->NoWaitPriortySend(static_pointer_cast<Host>(shared_from_this()), sendBuffer);
}
bool Host::CanRPCT()
{
	LONGLONG curr = GetTickCount64();
	LONGLONG recv = _RPCT_recv_Time.load();
	_RPCT_recv_Time.store(curr);
	if (curr - recv > RPCT_ABLE_TIME)
		return true;
	return false;
}
