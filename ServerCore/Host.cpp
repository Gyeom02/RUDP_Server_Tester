#include "pch.h"
#include "Host.h"

Host::Host(int32 id)
	: client_Id(id) {
	_tickConnectManager = MakeShared<TickConnectManager>(id, GTransportControl.GetTickTimerWheel());
	_tickConnectManager->Init();
	deliveryManager = MakeShared<DeliveryNotificationManager>();
}
void Host::Send(SendBufferRef sendBuffer)
{
	//WRITE_LOCK;

	ownerSocket->Send(shared_from_this(), sendBuffer);
}

void Host::PriortySend(shared_ptr<vector<SendBufferRef>> sendBuffers)
{
	ownerSocket->PriortySend(shared_from_this(), sendBuffers);
}
void Host::ControlSend(SendBufferRef sendBuffer)
{
	//WRITE_LOCK;
	ownerSocket->ControlSend(shared_from_this(), sendBuffer);
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

void Host::HandleACK(double rtt)
{
	rttManager.HandleACK(rtt);
}

void Host::PushControlJob(CallbackType&& func)
{
	_controlJobs.PushJob(move(func));

	bool expected = false;
	if (_controlJobs.bInsertReadyQueue.compare_exchange_strong(expected, true))
	{
		GTransportControl.PushHostControlJobReady(shared_from_this());
		GTransportControl.GetLazyAssist()._jobCv.notify_one();
	}
}

