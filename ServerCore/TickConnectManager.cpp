#include "pch.h"
#include "TickConnectManager.h"

TickConnectManager::TickConnectManager(int32 clientid, UTime::TimeWheel& wheel)
	: client_id(clientid), _wheel(wheel), _lastRecvTime(0), _lastSendTime(0)
{
	_state = ConnectState::Connect;
	
	
}
TickConnectManager::~TickConnectManager()
{
	_wheel.RemoveTimer(&_timerNode);
	//_timerNode.owner = nullptr;
}
void TickConnectManager::Init()
{
	_timerNode.owner = static_pointer_cast<TickConnectManager>(shared_from_this());
	ScheduleNextTick(ComputeNextTickDelay());
}
void TickConnectManager::ScheduleNextTick(uint64 delayMs)
{
	_wheel.RemoveTimer(&_timerNode);
	_wheel.AddTimer(&_timerNode, delayMs);
}

void TickConnectManager::Tick(uint64 nowUs)
{
	TickStateMachine(nowUs); // Handle Connect State

	if (_state != ConnectState::Disconnect)
		ScheduleNextTick(ComputeNextTickDelay());
}

void TickConnectManager::UpdateLastSendTime()
{
	_lastSendTime.store(UTime::GetNow());
}
void TickConnectManager::UpdateLastRecvTime()
{
	_lastRecvTime.store(UTime::GetNow());
}

void TickConnectManager::TickStateMachine(uint64 nowUs)
{
	switch (_state.load())
	{
	case ConnectState::Connect:
		HandleConnectState(nowUs);
		break;
	case ConnectState::ClosedByPeer:
		OnDisconnect();
		break;
	case ConnectState::ProblemDetected:
		OnDisconnect();
		break;
	case ConnectState::Disconnect:
		break;

	default:
		break;
	}
}

void TickConnectManager::HandleConnectState(uint64 nowUs)
{
	CheckKeepAlive(nowUs);
	CheckTimeOut(nowUs);
	CheckPeriodSyncRWind(nowUs);
}

void TickConnectManager::CheckKeepAlive(uint64 nowUs)
{
	//cout << "CheckKeepAlive" << endl;
	if (_lastSendTime.load() == 0 || _lastSendTime.load() > nowUs)
	{
		_lastSendTime = UTime::GetNow();
		return;
	}
	if ((nowUs - _lastSendTime.load()) > _setting_config.keepAliveIntervalUs)
	{
		GTransportControl.OnSendPing(client_id);
	}

}

void TickConnectManager::CheckTimeOut(uint64 nowUs)
{
	//cout << "CheckTimeOut" << endl;
	if (_lastRecvTime.load() == 0 || _lastRecvTime.load() > nowUs)
	{
		_lastRecvTime.store(UTime::GetNow());
		return;
	}
	if ((nowUs - _lastRecvTime.load()) > _setting_config.connectionTimeoutUs)
	{
		OnDetectedProblem("Recv TimeOut");
	}
	//cout << "CheckTimeOut now - lastrecv: " << nowUs - _lastRecvTime.load() << " | connectionTimeoutUs : " << _setting_config.connectionTimeoutUs << endl;

}

void TickConnectManager::CheckPeriodSyncRWind(uint64 nowUs)
{
	if (_lastSyncRWind == 0 || _lastSyncRWind > nowUs)
	{
		_lastSyncRWind = UTime::GetNow();
		return;
	}
	if ((nowUs - _lastSyncRWind) > _setting_config.periodSyncRWindUs)
	{

		GTransportControl.OnPeriodicRwindSync(client_id);


		_lastSyncRWind = nowUs;
		//OnDetectedProblem("Recv TimeOut");
	}
}

void TickConnectManager::OnDisconnect()
{
	//TODO CleanUp
	_state.store(ConnectState::Disconnect);

	int32 clientid = client_id;
	GHostManager.Remove(clientid);
	// ID Reuse Setting
	GHostManager.PushID(clientid);
	GQoS->ErasePlayer(clientid);

	
}