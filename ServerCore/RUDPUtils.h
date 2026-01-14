#pragma once
namespace LazyAssist
{
	enum
	{
		NORMAL_TickMs = 10,
	};
}
struct LazyWorkAssist
{
	LazyWorkAssist(uint32 tickMs = LazyAssist::NORMAL_TickMs) : _tickMs(tickMs) {}

	condition_variable _jobCv;
	Mutex _jobMutex;

	const uint32 _tickMs = 10; // 10ms
	std::chrono::steady_clock::time_point _nextTick;

	void SetNextTickFromNow() { _nextTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(_tickMs); }
};


