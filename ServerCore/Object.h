#pragma once
class Object : public enable_shared_from_this<Object>
{
private:
	enum
	{
		RPCT_ABLE_TIME = 30,
		
	};
public:
	Object() {}
	virtual ~Object() {}

	virtual void PriortySend(SendBufferRef sendBuffer) { cout << "object Send" << endl; }

	bool CanRPCT();
private:
	atomic<uint64> _RPCT_recv_Time;
};

using ObjectRef = shared_ptr<Object>;