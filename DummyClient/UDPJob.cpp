#include "pch.h"
#include "UDPJob.h"
UDPJob GUDPJob;

void UDPJob::Push(JobFunc job)
{
	WRITE_LOCK;
	jobQueue.push(job);
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
	if (jobQueue.empty())
		return nullptr;
	JobFunc func = jobQueue.front();
	jobQueue.pop();
	return func;
}
