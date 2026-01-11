#pragma once
class Object;

class ObjectManager
{
public:

	void Add(int32 id, ObjectRef player);
	void Remove(int32 id);
	void BroadCast(SendBufferRef sendBuffer);
	ObjectRef GetPlayer(int32 id);
	Map<int32, ObjectRef> GetPlayers() { USE_LOCK;  return _players; }
	/* Reuse ID Function */
	int32 ReuseID();
	void PushID(int32 id);

private:
	USE_LOCK;
	Map<int32, ObjectRef> _players;
	PriorityQueue<int32, vector<int32>, greater<int32>> _listOfReuseID;
};

extern ObjectManager GObjectManager;