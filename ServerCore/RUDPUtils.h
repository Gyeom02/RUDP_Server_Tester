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
		return std::chrono::duration_cast<chrono::microseconds>(steady_clock::now().time_since_epoch()).count(); // return microseconds
	}
	static uint64 GetNow_Ms()
	{
		return std::chrono::duration_cast<chrono::milliseconds>(steady_clock::now().time_since_epoch()).count(); // return miliseconds
	}
		
	const uint64 Wheel_Resolution_ms = 10;
	const uint32 Wheel_Size = 4096;

	class TimeWheelManager : public enable_shared_from_this<TimeWheelManager>
	{
	public:
		TimeWheelManager() {}
		~TimeWheelManager() {}

		virtual void Tick(uint64 nowUs) abstract;
	};

	struct TimerNode
	{
		
		uint64 expireTick;
		weak_ptr<TimeWheelManager> owner;
		TimerNode* prev = nullptr;
		TimerNode* next = nullptr;
	};
	struct TimerSlot
	{
		TimerNode* head = nullptr;
	};

	
	class TimeWheel
	{
	private:
		

		
	public:
		TimeWheel() { _slots.resize(Wheel_Size); _currentTick = GetNowTick(); }

		void AddTimer(TimerNode* node, uint64 delayMs)
		{
			uint64 ticks = delayMs / Wheel_Resolution_ms;
			if (ticks == 0) ticks = 1;

			uint64 expire = _currentTick + ticks;
			uint32 slot = expire % Wheel_Size;

			node->expireTick = expire;
			InsertNode(&_slots[slot], node);
		}

		void RemoveTimer(TimerNode* node)
		{
			if (!node->prev && !node->next) return;
			RemoveNode(_slots[(node->expireTick % Wheel_Size)], node);
		}

		void Tick();
		
	private:
		void ProcessSlot(TimerSlot& slot)
		{
			TimerNode* node = slot.head;
			while (node)
			{
				TimerNode* next = node->next;
				if (node->expireTick <= _currentTick)
				{
					RemoveNode(_slots[(node->expireTick % Wheel_Size)], node);

					auto owner_ptr = node->owner.lock();
					if (owner_ptr)
						owner_ptr->Tick(UTime::GetNow());
				}
				node = next;
			}
		}
		static void InsertNode(TimerSlot* slot, TimerNode* node)
		{
			node->prev = nullptr;
			node->next = slot->head;
			if(slot->head)
				slot->head->prev = node;
			slot->head = node;
		}
		static void RemoveNode(TimerSlot& slot, TimerNode* node)
		{
			if (node->prev)
				node->prev->next = node->next;
			else // slot->head = this
				slot.head = nullptr;
			if (node->next)
				node->next->prev = node->prev;
			node->prev = node->next = nullptr;

			
		}
		static uint64 GetNowTick()
		{
			return UTime::GetNow_Ms() / Wheel_Resolution_ms;
		}
	private:
		vector<TimerSlot> _slots;
		uint64 _currentTick;
	};
}

namespace SPSC
{
	
#ifdef _DEBUG
	const int32 ccapacity = 1024;
#else
	const int32 ccapacity = 256;
#endif
	template<typename T, int32 _Capacity = ccapacity>
	class UQueue
	{
		static_assert((_Capacity & (_Capacity - 1)) == 0,
			"Capacity must be power of 2");
	public:
		UQueue() : _pushIdx(0), _popIdx(0) {}
		~UQueue() {}

		void Push(const T& v);
		T Pop();
		T Front();
		int32 Size();
		bool Empty();

	private:
		atomic<uint32> _pushIdx; 
		atomic<uint32> _popIdx;

		T _buffer[_Capacity];
	};
	
}


template<typename T, int32 _Capacity>
inline void SPSC::UQueue<T, _Capacity>::Push(const T& v)
{
	const uint32 nowidx = _pushIdx.load();
	const uint32 next = (nowidx + 1) % _Capacity;
	if (next == _popIdx.load())
		CRASH("SPSCQueue Full of Queue");
	_buffer[nowidx] = v;
	_pushIdx.store(next);
}

template<typename T, int32 _Capacity>
inline T SPSC::UQueue<T, _Capacity>::Pop()
{
	const uint32 now_popidx = _popIdx.load();
	const uint32 now_pushidx = _pushIdx.load();
	if (now_popidx == now_pushidx)
		CRASH("SPSCQueue Empty Pop");
	_popIdx.store((now_popidx + 1) % _Capacity);
	return _buffer[now_popidx];
}

template<typename T, int32 _Capacity>
inline T SPSC::UQueue<T, _Capacity>::Front()
{
	if (Empty())
		CRASH("SPSCQueue Empty");
	return _buffer[_popIdx.load()];
}

template<typename T, int32 _Capacity>
inline int32 SPSC::UQueue<T, _Capacity>::Size()
{
	return _pushIdx.load() - _popIdx.load();
}

template<typename T, int32 _Capacity>
inline bool SPSC::UQueue<T, _Capacity>::Empty()
{
	if (_popIdx.load() == _pushIdx.load())
		return true;
	return false;
}

