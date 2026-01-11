#pragma once
class Object : public enable_shared_from_this<Object>
{
private:
	enum
	{
		RPCT_ABLE_TIME = 30,
		
	};
public:
	Object() { deliveryManager = MakeShared<DeliveryNotificationManager>(); }
	virtual ~Object() {}

	virtual void PriortySend(SendBufferRef sendBuffer) { cout << "object Send" << endl; }

	bool CanRPCT();

	/* DeliveryManager */
	int32 GetExpectedSeqNum() { return GetDeliveryManager()->GetExpectedSeqNum(); }

	DeliveryManagerRef GetDeliveryManager() { return deliveryManager; }

	int32 GetFragmentPrimID() { int32 id = _giveFragmentID.fetch_add(1); return id; }

	int32 GetRWind() { return GetDeliveryManager()->GetRWind(); }
private:
	atomic<uint64> _RPCT_recv_Time;

	DeliveryManagerRef deliveryManager;

	atomic<int32> _giveFragmentID = 0;
};

using ObjectRef = shared_ptr<Object>;