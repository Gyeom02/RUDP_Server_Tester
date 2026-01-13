#include "pch.h"
#include "HostManager.h"
//#include "O.h"
HostManager GHostManager;

void HostManager::Add(int32 id, HostRef player)
{
	WRITE_LOCK;
	_players[id] = player;
	//_players.insert(make_pair(id, player));
}

void HostManager::Remove(int32 id)
{
	WRITE_LOCK;
	_players.erase(id);
}

void HostManager::BroadCast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto player : _players)
	{
		player.second->Send(sendBuffer);
	}
}

HostRef HostManager::GetPlayer(int32 id)
{
	WRITE_LOCK;

	auto player = _players.find(id);
	if (player == _players.end()) // 존재하지않음
		return nullptr;
	
	return (player->second);
}


int32 HostManager::ReuseID()
{
	WRITE_LOCK;

	if (_listOfReuseID.empty())
		return -1;

	int32 id = _listOfReuseID.top();
	_listOfReuseID.pop();

	return id;
}

void HostManager::PushID(int32 id)
{
	WRITE_LOCK;

	_listOfReuseID.push(id);
}