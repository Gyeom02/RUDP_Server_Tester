#include "pch.h"
#include "RUDPUtils.h"


template<typename T, int32 _Capacity>
void SPSC::Queue<T, _Capacity>::Push(const T& v)
{
	const uint32 nowidx = _pushIdx.load();
	const uint32 next = (nowidx + 1) % _Capacity;
	if (next == _popIdx.load())
		CRASH("SPSCQueue Full of Queue");
	_buffer[nowidx] = v;
	_pushIdx.store(next);
}

template<typename T, int32 _Capacity>
T SPSC::Queue<T, _Capacity>::Pop()
{
	const uint32 now_popidx = _popIdx.load();
	const uint32 now_pushidx = _pushIdx.load();
	if (now_popidx == now_pushidx)
		CRASH("SPSCQueue Empty Pop");
	_popIdx.store((now_popidx + 1) % _Capacity);
	return _buffer[now_popidx];
}

template<typename T, int32 _Capacity>
bool SPSC::Queue<T, _Capacity>::Empty()
{
	if (_popIdx.load() == _pushIdx.load())
		return true;
	return false;
}
