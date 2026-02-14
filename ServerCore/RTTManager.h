#pragma once
class RTTManager
{
	enum QueueDelay_Enum
	{
		//UNDERUTILIZED = 3000,
		BUILDUP = 5000,
		CONGESTED = 15000,
		DRAINING = 0,
	};
	enum
 	{
		IDLE_FLAG = 0,
		CONGEST_FLAG = 1,
		NULL_FLAG = 2,
	};


	struct RecentRTT
	{
		uint64 _when_made = 0; // microseconds
		double _rtt_us = 0.0f;
	};
public:
	RTTManager();
	~RTTManager();

	

	static uint64 GetRTT(uint64 time_stamp); // return microseconds
	
	uint64 GetRTT_milsec(uint64 time_stamp); // return milliseconds;

	void HandleACK(double rtt);
	void UpdatePaceRate(double newRTT);
	void UpdateCongestBudget();
	bool ValidBudget(int32 packetSize);
	void UseBudget(int32 packetSize);

	void UpdateRttQueue(double queueDelay); //참조만을 인자로 받음 복사 배제

	double GetResendDelay(int32 retransCount = 0);

	double GetRTO() { return _rto.load(); }
private:
	double _baseRTT; // RTT when qeueuing is zero or very low
	double _oldSRTT; // recent SRTT;
	//double _queueDelay; // SRTT - BaseRTT;
	double _rttGradient; // prevSRTT - oldSRTT; // + = 송수신 흐름 안좋아지는 중 , - = 송수신 흐름 좋아지는 중
	atomic<double> _rttVar = 0.0f;
	atomic<double> _paceRateBytePerSec = 0.0;
	atomic<double> _paceSendBudget = 0.0;
	atomic<double> _rto = 0.0f;

	const double _minSendRate = 10 * 1024;
	const double _maxSendRate = 50 * 1024 * 1024;
	const double _InitSendRate = 24 * 1024;
	const double alpha = 0.1;
	
	const double _rtoMin = 10'000.0; // 10ms
	const double _rtoMax = 60'000'000.0; // 60s -> 1min
	const double _rtoK = 4.0;
	const double _rtoG = 1'000.0; // 1ms Granularity
	deque<RecentRTT> _recentRttsQueue;

	uint64 _lastUpdateBudgetUs = 0;

	const uint64 RttQueueValidTime = 5'000'000; // microseconds 기준 5s
	const size_t kMaxRttSamples = 128;

	const float Congest_Rate = 0.85f;
	const float BuildUp_Rate = 0.98f;
	const float Recover_Rate = 1.05f;

	const uint64 Reasonable_RTT_Us = 500'000; 
public:
	double GetRTTGradient() { return _rttGradient; }
	double GetRTTPaceRate() { return _paceRateBytePerSec.load(); }
	double GetRTTBudget() { return _paceSendBudget.load(); }

public:
	atomic<double> _debugQueueDelay; // Only For Debug
	string GetRttState(); // Only For Debug
};

