#pragma once
class Player;

class PlayerManager
{
public:

	void Add(int32 id, PlayerRef player);
	void Remove(int32 id);
	void BroadCast(SendBufferRef sendBuffer);
	PlayerRef GetPlayer(int32 id);
	Map<int32, PlayerRef>& GetPlayers() { return _players; }
	/* Reuse ID Function */
	int32 ReuseID();
	void PushID(int32 id);

private:
	USE_LOCK;
	Map<int32, PlayerRef> _players;
	PriorityQueue<int32, vector<int32>, greater<int32>> _listOfReuseID;
};

extern PlayerManager GPlayerManager;