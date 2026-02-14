#pragma once

using JobFunc = std::function<void()>;

using PacketHandleFunc = bool (*)(shared_ptr<UDPSocket>, NetAddress, BYTE*, int32);
 
class UDPRecvHandler 
{
public:
	UDPRecvHandler(QoSShard* _ownerShard, PacketHandleFunc func);
	~UDPRecvHandler();

	//void PushSend(JobFunc job, int priority);
	void CreateAsyncWorkThread();
	void DOWork();
	//void Push(const JobFunc& _job);
	//JobFunc Pop();
	void HandleRecvPacket(std::shared_ptr<QoSPlayer> _Player);

	void StopThread();
	//void SleepTillGetSignal();
	//void SetSignal();
private:
	USE_LOCK;
//	Queue<JobFunc> JobQueue;
	atomic<bool> _continue = false;

	QoSShard* _shard;

	//condition_variable _cv;
	//mutex _mutex;
	//int32 _start;
	//int32 _end;
//	bool _shutdown = false;
	PacketHandleFunc _func;

	//LazyWorkAssist _lazyAssist;
};

using UDPRecvHandlerRef = std::shared_ptr<UDPRecvHandler>;