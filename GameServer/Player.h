#pragma once
#include "UDP.h"
#include "Object.h"
using PlayerRef = shared_ptr<class Player>;
struct Position
{
	Position(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {  }
	Position& operator=(const Position& pos) 
	{ 
		x = pos.x.load(); 
		y = pos.y.load();
		z = pos.z.load();
		return *this;
	}
	atomic<float> x;
	atomic<float> y;
	atomic<float> z;
};
struct Rotation
{
	Rotation(float _pitch = 0, float _yaw = 0, float _roll = 0) : pitch(_pitch), yaw(_yaw), roll(_roll) {  }
	Rotation& operator=(const Rotation& pos)
	{
		pitch = pos.pitch.load();
		yaw = pos.yaw.load();
		roll = pos.roll.load();
		return *this;
	}
	atomic<float> pitch;
	atomic<float> yaw;
	atomic<float> roll;
};
struct Velocity
{
	Velocity(float _vx = 0, float _vy = 0, float _vz = 0) : vx(_vx), vy(_vy), vz(_vz) {  }
	Velocity& operator=(const Velocity& pos)
	{
		vx = pos.vx.load();
		vy = pos.vy.load();
		vz = pos.vz.load();
		return *this;
	}
	atomic<float> vx;
	atomic<float> vy;
	atomic<float> vz;
};

class Player : public Object
{
public:
	Player(int32 id);
	virtual ~Player() { cout << "Player ID : " << playerId << " Erased" << endl; } // 메모리 누수 확인

	 void Send(SendBufferRef sendBuffer);
	virtual void PriortySend(SendBufferRef sendBuffer) override;
	void SetImport(Position _pos, Rotation _rot, Velocity _vel);
	void SetNickName(string nickname);
	string GetNickName();

	atomic<int32>					playerId = 0;
	atomic<int32>					roomId	 = -1;
	atomic<int32>					roomprimid = -1;
	atomic<int32>					teamNum = -1;
	
	//Protocol::PlayerType	type = Protocol::PLAYER_TYPE_NONE;
	NetAddress				netAddress;
//	GameSessionRef			ownerSession; // Cycle
	UDPSocketPtr			ownerSocket; // Cycle

	Position pos;
	Rotation rot;
	Velocity vel;

	bool bready = false;

/* DeliveryManager */
	DeliveryManagerRef GetDeliveyManager() { return deliveryManager; }

private:
	USE_LOCK;
	string	_nickname;
	DeliveryManagerRef deliveryManager;
};

