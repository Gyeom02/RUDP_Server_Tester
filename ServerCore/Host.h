#pragma once
#include "RTTManager.h"
#include "TickConnectManager.h"

class ControlJobs
{
public:
	friend class Host;

	bool Execute();
	bool IsEmpty() { READ_LOCK; return _jobs.empty(); }

	bool CheckHostControlJobEmpty();

private:
	void PushJob(CallbackType&& callback);

	queue<JobRef> _jobs;

	atomic<bool> bInsertReadyQueue = false;
	USE_LOCK;
};

class Host : public enable_shared_from_this<Host>
{
private:
	enum
	{
		RPCT_ABLE_TIME = 30,
		
	};
public:
	Host(int32 id);
	virtual ~Host() {}

	void InitDeliveryManager() { deliveryManager->SetOwner(shared_from_this()); }

	void Send(SendBufferRef sendBuffer);
	//void PriortySend(SendBufferRef sendBuffer);
	void PriortySend(shared_ptr<vector<SendBufferRef>> sendBuffers);
	void ControlSend(SendBufferRef sendBuffer); // Only For Control Packet Send

	bool CanRPCT();

	/* DeliveryManager */
	int32 GetRWindExpectedSeqNum() { return GetDeliveryManager()->GetRWindExpectedSeqNum(); }

	DeliveryManagerRef GetDeliveryManager() { return deliveryManager; }

	int32 GetFragmentPrimID() { int32 id = _giveFragmentID.fetch_add(1); return id; }

	int32 GetRWind() { return GetDeliveryManager()->GetRWind(); }

	RTTManager& GetRTTManager() { return rttManager; }
	void HandleACK(double rtt);

	shared_ptr<TickConnectManager> GetConnectManager() { return _tickConnectManager; }
	void UpdateLastSendTime() { _tickConnectManager->UpdateLastSendTime(); }
	void UpdateLastRecvTime() { _tickConnectManager->UpdateLastRecvTime(); }
public: //ControlJob 
	ControlJobs& GetControlJobs() { return _controlJobs; }
	void PushControlJob(CallbackType&& func);

public:
	NetAddress				netAddress;
	//	GameSessionRef			ownerSession; // Cycle
	shared_ptr<class UDPSocket>			ownerSocket; // Cycle
	atomic<int32>					client_Id = 0;

	LONGLONG curRwindTimeStamp = 0;

protected:
	//USE_LOCK;
private:
	
	atomic<uint64> _RPCT_recv_Time;

	DeliveryManagerRef deliveryManager;

	atomic<int32> _giveFragmentID = 0;

	RTTManager rttManager;

	ControlJobs _controlJobs;

	shared_ptr<TickConnectManager> _tickConnectManager;
};

using HostRef = shared_ptr<Host>;