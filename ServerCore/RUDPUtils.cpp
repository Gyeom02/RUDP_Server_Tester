#include "pch.h"
#include "RUDPUtils.h"

void UTime::TimeWheel::Tick()
{
	//cout << " Tick " << endl;
	uint64_t nowTick = GetNowTick();

	while (_currentTick <= nowTick)
	{
			
		uint32_t slot = _currentTick % Wheel_Size;
		ProcessSlot(_slots[slot]);
		_currentTick++;
	}
}