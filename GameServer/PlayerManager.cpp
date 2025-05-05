#include "pch.h"
#include "PlayerManager.h"
#include "Player.h"
PlayerManager GPlayerManager;

void PlayerManager::Add(int32 id, PlayerRef player)
{
	WRITE_LOCK;
	_players[id] = player;
	//_players.insert(make_pair(id, player));
}

void PlayerManager::Remove(int32 id)
{
	WRITE_LOCK;
	_players.erase(id);
}

void PlayerManager::BroadCast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto player : _players)
	{
		player.second->Send(sendBuffer);
	}
}

PlayerRef PlayerManager::GetPlayer(int32 id)
{
	WRITE_LOCK;

	auto player = _players.find(id);
	if (player == _players.end()) // 존재하지않음
		return nullptr;
	
	return (player->second);
}


int32 PlayerManager::ReuseID()
{
	WRITE_LOCK;

	if (_listOfReuseID.empty())
		return -1;

	int32 id = _listOfReuseID.top();
	_listOfReuseID.pop();

	return id;
}

void PlayerManager::PushID(int32 id)
{
	WRITE_LOCK;

	_listOfReuseID.push(id);
}