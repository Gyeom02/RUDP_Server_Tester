#include "pch.h"
#include "Object.h"

void Object::Send(SendBufferRef sendBuffer)
{
	WRITE_LOCK;

	ownerSocket->Send(static_pointer_cast<Object>(shared_from_this()), sendBuffer);
}

void Object::PriortySend(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	ownerSocket->PriortySend(static_pointer_cast<Object>(shared_from_this()), sendBuffer);
}
bool Object::CanRPCT()
{
	LONGLONG curr = GetTickCount64();
	LONGLONG recv = _RPCT_recv_Time.load();
	_RPCT_recv_Time.store(curr);
	if (curr - recv > RPCT_ABLE_TIME)
		return true;
	return false;
}
