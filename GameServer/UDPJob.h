#pragma once
using JobFunc = std::function<void()>;

class UDPJob
{
public:
	UDPJob() {};
	~UDPJob() {};

	void Push(JobFunc job);
	void DOJob();
	JobFunc Pop();
private:
	USE_LOCK;
	Queue<JobFunc> jobQueue;
};

extern UDPJob GUDPJob;
