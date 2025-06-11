#include "pch.h"
#include "QoSCore.h"

QoSCore GQoS;
void QoSCore::Push(ObjectRef object, SendBufferRef packet)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet->Buffer());

	uint16 priority = header->priority;
	if (priority == Priority::HIGH)
	{
		WRITE_LOCK;
		_queues[Priority::HIGH].push(MakeShared<SavedPacket>(object, packet));
	}
	else if (priority == Priority::MEDIUM)
	{
		WRITE_LOCK;
		_queues[Priority::MEDIUM].push(MakeShared<SavedPacket>(object, packet));
	}
	else if (priority == Priority::LOW)
	{
		WRITE_LOCK;
		_queues[Priority::LOW].push(MakeShared<SavedPacket>(object, packet));
	}
	else
		return;
}

void QoSCore::DoWork()
{
	while (true)
	{
		//cout << " QoSCore::DoWork" << endl;
		PopSend();
	}
}


void QoSCore::PopSend()
{
	shared_ptr<SavedPacket> savePacket = nullptr; //기아 현상 해결해야함
	{
		WRITE_LOCK;
		for (int16 i = Priority::HIGH; i < Priority::MAX; i++)
		{

			if (!_queues[i].empty())
			{
				savePacket = _queues[i].front();
				_queues[i].pop();
				break;
			}
		}
	}
	if (savePacket == nullptr)
		return;
	PacketHeader* header = reinterpret_cast<PacketHeader*>(savePacket->sendBuffer->Buffer());

	savePacket->object->PriortySend(savePacket->sendBuffer);
	

}
