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

namespace UTime
{
	using steady_clock = chrono::steady_clock;

	static uint64 GetNow()
	{
		return std::chrono::duration_cast<chrono::microseconds>(steady_clock::now().time_since_epoch()).count();
	}
		// return microseconds
}

namespace SPSC
{
	
#ifdef _DEBUG
	const int32 ccapacity = 1024;
#else
	const int32 ccapacity = 256;
#endif
	template<typename T, int32 _Capacity = ccapacity>
	class Queue
	{
		static_assert((_Capacity& (_Capacity - 1)) == 0,
			"Capacity must be power of 2");
	public:
		Queue() : _pushIdx(0), _popIdx(0) {}
		~Queue() {}

		void Push(const T& v);
		T Pop();
		bool Empty();

	private:
		atomic<uint32> _pushIdx; 
		atomic<uint32> _popIdx;

		T _buffer[_Capacity];
	};
	
}