#include "pch.h"
#include "Object.h"

bool Object::CanRPCT()
{
	LONGLONG curr = GetTickCount64();
	LONGLONG recv = _RPCT_recv_Time.load();
	_RPCT_recv_Time.store(curr);
	if (curr - recv > RPCT_ABLE_TIME)
		return true;
	return false;
}
