#pragma once


enum ConnectState : uint8
{
	Connecting,
	Connect,
	ClosedByPeer,
	ProblemDetected,
	Disconnect
};

class TickConnectManager : public UTime::TimeWheelManager
{
private:
	struct Config
	{
		uint64 keepAliveIntervalUs = 3'000'000; //3s
		uint64 connectionTimeoutUs = 15'000'000; //60s -> 1min
		uint64 periodSyncRWindUs = 1'000'000; // 1s
		uint32 maxRetransmitFail = 0; // 0 means no limit to Retransmit 0 < n -> n is limit of retransmit per host
		uint64 rttExtremeDuration = 0;
	};
public:
	TickConnectManager(int32 clientid, class UTime::TimeWheel& wheel);
	~TickConnectManager();
	void ScheduleNextTick(uint64 delayMs);
	
	void Init();
	virtual void Tick(uint64 nowUs);
	
	void UpdateLastSendTime();
	
	void UpdateLastRecvTime();
	
private:
	uint64 ComputeNextTickDelay()
	{
		if (_state.load() == ConnectState::Connect)
			return 50;
		return 1000;
	}
	void TickStateMachine(uint64 nowUs);
	
	void HandleConnectState(uint64 nowUs);
	
	void CheckKeepAlive(uint64 nowUs);
	void CheckTimeOut(uint64 nowUs);
	void CheckPeriodSyncRWind(uint64 nowUs);

	void OnDisconnect();
	
public:
	void OnDetectedProblem(string log) // it'is public 'cause user layer needs to use this func
	{
		_state.store(ConnectState::ProblemDetected);
		cout << "Client ID : " << client_id << " Disconnect ProblemDetected : " << log << endl;
	}
	void OnClosedByPeer()
	{
		_state.store(ConnectState::ClosedByPeer);
		cout << "Client ID : " << client_id << " Disconnect ClosedByPeer " << endl;
	}
	Config& GetConfig() { return _setting_config; }
private:

	int32 client_id = -1;
	class UTime::TimeWheel& _wheel;
	struct UTime::TimerNode _timerNode;
	atomic<ConnectState> _state;
	atomic<uint64> _lastRecvTime = 0; // us
	atomic<uint64> _lastSendTime = 0; // us
	uint64 _lastSyncRWind = 0;
	Config _setting_config;
};
