#pragma once
//Class For Quality of Service
#include "Nids.h"
#include <map>
#include "Object.h"
#include "Lock.h"

class QoSCore
{
public:
	enum Priority: uint16
	{
		HIGH = 0,
		MEDIUM = 1,
		LOW = 2,
		FPC = 3, //Fastest Possibly Can -> Usual Used For Replication Packets
		MAX = 3,
	};
	
	void Push(ObjectRef object, SendBufferRef packet);
	void DoWork();
private:
	struct SavedPacket
	{
		SendBufferRef sendBuffer;
		ObjectRef object;
		SavedPacket(ObjectRef s, SendBufferRef buffer) : object(s), sendBuffer(buffer) {}
	};
	void PopSend();
	unordered_map<int16, queue<shared_ptr<SavedPacket>>> _queues;
	USE_LOCK;

};

extern QoSCore GQoS;