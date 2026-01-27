#pragma once
#include "RUDPWindow.h"

class SendBufferChunk;

/*----------------
	SendBuffer
-----------------*/

class SendBuffer
{
public:
	SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize);
	~SendBuffer();

	BYTE*		Buffer() { return _buffer; }
	uint32		AllocSize() { return _allocSize; }
	uint32		WriteSize() { return _writeSize; }
	void		Close(uint32 writeSize);

public: // For RUDP
	bool IsGotAck() { return _bAcked.load(); }
	void GotAck() { SetbAcked(true); }
private:
	void SetbAcked(bool bAck) { _bAcked.store(bAck); }
private:
	BYTE*				_buffer;
	uint32				_allocSize = 0;
	uint32				_writeSize = 0;
	SendBufferChunkRef	_owner;
	atomic<bool> _bAcked = false;

};

/*--------------------
	SendBufferChunk
--------------------*/

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum
	{
		SEND_BUFFER_CHUNK_SIZE = RUDPWIND::SEND_BUFFER_CHUNK_SIZE,
	};

public:
	SendBufferChunk();
	~SendBufferChunk();

	void				Reset();
	SendBufferRef		Open(uint32 allocSize);
	void				Close(uint32 writeSize);

	bool				IsOpen() { return _open; }
	BYTE*				Buffer() { return &_buffer[_usedSize]; }
	uint32				FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; }

private:
	Array<BYTE, SEND_BUFFER_CHUNK_SIZE>		_buffer = {};
	bool									_open = false;
	uint32									_usedSize = 0;
};

/*---------------------
	SendBufferManager
----------------------*/

class SendBufferManager
{
public:
	SendBufferRef		Open(uint32 size);

private:
	SendBufferChunkRef	Pop();
	void				PushSend(SendBufferChunkRef buffer);

	static void			PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	Vector<SendBufferChunkRef> _sendBufferChunks;
};
