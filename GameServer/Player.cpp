#include "pch.h"
#include "Player.h"

Player::Player(int32 id) 
	: playerId(id), roomId(-1), roomprimid(-1), teamNum(-1)
{ 
	deliveryManager = MakeShared<DeliveryNotificationManager>();
}

void Player::Send(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	
	ownerSocket->Send(static_pointer_cast<Player>(shared_from_this()), sendBuffer);
}

void Player::PriortySend(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	ownerSocket->PriortySend(static_pointer_cast<Player>(shared_from_this()), sendBuffer);
}

void Player::SetImport(Position _pos, Rotation _rot, Velocity _vel)
{
	pos = _pos;
	rot = _rot;
	vel = _vel;
}

void Player::SetNickName(string nickname)
{
	WRITE_LOCK;
	_nickname = nickname;
}

string Player::GetNickName()
{
	READ_LOCK;
	return _nickname;
}
