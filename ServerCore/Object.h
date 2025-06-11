#pragma once
class Object : public enable_shared_from_this<Object>
{
public:
	Object() {}
	virtual ~Object() {}

	virtual void PriortySend(SendBufferRef sendBuffer) { cout << "object Send" << endl; }
};

using ObjectRef = shared_ptr<Object>;