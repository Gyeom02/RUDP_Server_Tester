#pragma once
class Host;

class HostManager
{
public:

	void Add(int32 id, HostRef player);
	void Remove(int32 id);
	void BroadCast(SendBufferRef sendBuffer);
	HostRef GetPlayer(int32 id);
	Map<int32, HostRef> GetPlayers() { USE_LOCK;  return _players; }
	/* Reuse ID Function */
	int32 ReuseID();
	void PushID(int32 id);

private:
	USE_LOCK;
	Map<int32, HostRef> _players;
	PriorityQueue<int32, vector<int32>, greater<int32>> _listOfReuseID;
};

extern HostManager GHostManager;