#include "pch.h"
#include "UDPJob.h"
UDPJob GUDPJob;

void UDPJob::Push(JobFunc job, int priority)
{
	WRITE_LOCK;
	if (priority == QoSCore::HIGH)
	{
		highJobQueue.push(job);
	}
	else if (priority == QoSCore::LOW)
	{
		lowJobQueue.push(job);
	}
}

void UDPJob::DOJob()
{
	JobFunc func = Pop();
	if (func == nullptr)
		return;
	func();
}

JobFunc UDPJob::Pop()
{
	WRITE_LOCK;
	JobFunc func = nullptr;
	if (!highJobQueue.empty())
	{
		func = highJobQueue.front();
		highJobQueue.pop();
	}
	else if (!lowJobQueue.empty())
	{
		//cout << "LowJobQueue Size : " << lowJobQueue.size() << endl;
		func = lowJobQueue.front();
		lowJobQueue.pop();

	}
	return func;
}
