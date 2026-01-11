#include "pch.h"
#include "ObjectManager.h"
//#include "O.h"
ObjectManager GObjectManager;

void ObjectManager::Add(int32 id, ObjectRef player)
{
	WRITE_LOCK;
	_players[id] = player;
	//_players.insert(make_pair(id, player));
}

void ObjectManager::Remove(int32 id)
{
	WRITE_LOCK;
	_players.erase(id);
}

void ObjectManager::BroadCast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto player : _players)
	{
		player.second->Send(sendBuffer);
	}
}

ObjectRef ObjectManager::GetPlayer(int32 id)
{
	WRITE_LOCK;

	auto player = _players.find(id);
	if (player == _players.end()) // 존재하지않음
		return nullptr;
	
	return (player->second);
}


int32 ObjectManager::ReuseID()
{
	WRITE_LOCK;

	if (_listOfReuseID.empty())
		return -1;

	int32 id = _listOfReuseID.top();
	_listOfReuseID.pop();

	return id;
}

void ObjectManager::PushID(int32 id)
{
	WRITE_LOCK;

	_listOfReuseID.push(id);
}