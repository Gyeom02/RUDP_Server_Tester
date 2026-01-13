#include "pch.h"
#include "Player.h"

Player::Player(int32 id) 
	:  Host(id), roomId(-1), roomprimid(-1), teamNum(-1)
{ 
	
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
