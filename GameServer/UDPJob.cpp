#include "pch.h"
#include "UDPJob.h"
UDPJob GUDPJob;

void UDPJob::Push(JobFunc job, int priority)
{
	WRITE_LOCK;
	if(priority == QoSCore::HIGH)
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
	//	cout << "HighJobQueue Size : " << highJobQueue.size() << " -> ";
		func = highJobQueue.front();
		highJobQueue.pop();
	//	cout << highJobQueue.size() << endl;
	}
	else if (!lowJobQueue.empty())
	{
	//	cout << "LowJobQueue Size : " << lowJobQueue.size() << " -> ";
		func = lowJobQueue.front();
		lowJobQueue.pop();
	//	cout << lowJobQueue.size() << endl;
	}
	return func;
}
