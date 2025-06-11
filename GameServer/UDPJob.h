#pragma once
using JobFunc = std::function<void()>;

class UDPJob
{
public:
	UDPJob() {};
	~UDPJob() {};

	void Push(JobFunc job, int priority);
	void DOJob();
	JobFunc Pop();
private:
	USE_LOCK;
	Queue<JobFunc> highJobQueue;
	Queue<JobFunc> lowJobQueue;
};

extern UDPJob GUDPJob;
