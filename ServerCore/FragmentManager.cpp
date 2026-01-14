#include "pch.h"
#include "FragmentManager.h"


FragmentContext::FragmentContext(PacketHeader* header, uint32 primid, int16 frag_count, int16 original_size) // 처음 생성된거임
    : primID(primid), frag_Count(frag_count), original_Size(original_size)
    
{
    _recievedBitMap.assign(frag_Count, 0);
   // const PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
   // const FragmentHeader* FgHeader = reinterpret_cast<FragmentHeader*>(buffer + sizeof(PacketHeader));

   
    _storedBuffer.resize(sizeof(PacketHeader) + original_size);

    //nt32 offset = FgHeader->offset;
    memcpy(_storedBuffer.data(), header, sizeof(PacketHeader));
    stored_size += sizeof(PacketHeader);
   
    PacketHeader* _header = reinterpret_cast<PacketHeader*>(_storedBuffer.data());
    _header->size = original_size + sizeof(PacketHeader);
}

void FragmentContext::Store(BYTE* buffer, int32 size, int16 offset, int16 index)
{
    if (_recievedBitMap[index] == 1) // 이미 와있는 중복이다
    {
        //cout << "_recievedBitMap[index] == 1" << endl;
        return;
    }    
    memcpy(_storedBuffer.data() + sizeof(PacketHeader) + offset, buffer, size);

    stored_frag_count++;
    stored_size += size;
    _recievedBitMap[index] = 1;
    
}


bool Ordered_FG_Manager::OnOrderedRecv(uint32 seqNum, BYTE* buffer, int32 size, OUT queue<shared_ptr<FragmentContext>>& outReadyQueue)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    
    int32 payloadSize = header->size - sizeof(PacketHeader);
    if (header->bFragment != 1) // 조각화된 패킷이아님
    {
        
        const int32 seq_index = header->sn;
     //   cout << " const int32 seq_index = header->sn; : " << header->sn << endl;
        auto iter = _orderedCtxs.find(seq_index);
#ifdef _DEBUG
        if (iter != _orderedCtxs.end()) //존재한다
            _ASSERT(false);
#else
        
#endif
        //int16 primid, int16 frag_count, int16 original_size)
        shared_ptr<FragmentContext> ctx = make_shared<FragmentContext>(header, seq_index, 1, payloadSize);
       
        ctx->Store(buffer + sizeof(PacketHeader), payloadSize, 0, 0);

        //_fragCtxs.emplace(seq_index, ctx);
        _orderedCtxs[seq_index] = ctx;
        return IsThereReadyPacket(seqNum, outReadyQueue);
    }
    else
    {
        return OnFragment(seqNum, buffer, size, outReadyQueue);
    }
}

bool Ordered_FG_Manager::OnFragment(uint32 seqNum, BYTE* buffer, int32 size, OUT queue<shared_ptr<FragmentContext>>& outReadyQueue)
{

    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    FragmentHeader* FgHeader = reinterpret_cast<FragmentHeader*>(buffer + sizeof(PacketHeader));
    BYTE* payload = reinterpret_cast<BYTE*>(buffer + sizeof(PacketHeader) + sizeof(FragmentHeader));
    int32 payloadSize = FgHeader->size;
    uint32 seq_index = FgHeader->primID;
    //오염 체크
    
    
    if (FgHeader->frag_count <= 0)
        return false;
    if (FgHeader->index > FgHeader->frag_count)
        return false;
    if (FgHeader->size <= 0)
        return false;
    if (FgHeader->original_size <= FgHeader->size)
        return false;
    if (FgHeader->offset + FgHeader->size > FgHeader->original_size)
        return false;
  
    

    auto iter = _fragCtxs.find(seq_index);
    shared_ptr<FragmentContext> copy_ctx;
    if (iter == _fragCtxs.end()) // 처음 발견한 primID를 갖고있는 조각화 패킷을 발견
    {
        shared_ptr<FragmentContext> ctx = make_shared<FragmentContext>(header, seq_index, FgHeader->frag_count, FgHeader->original_size);
        //_fragCtxs[FgHeader->primID] = ctx;

        _fragCtxs[seq_index] = ctx;
       // auto [newIter, dummy] = _fragCtxs.emplace(seq_index, ctx);
        //iter = newIter;
        // 
        copy_ctx = ctx;
        // _fragCtxs.insert_or_assign(FgHeader->primID, ctx);
      
    }
    else
    {
        copy_ctx = iter->second;
    }
    //cout << "_fragCtxs.find(seq_index) : " << seq_index << endl;
    copy_ctx->_fragmentSeqNums.push(header->sn);

    copy_ctx->Store(payload, payloadSize, FgHeader->offset, FgHeader->index);
    //if (FgHeader->index > 0) //Head 패킷이 아님
    //{

    //}
    //else //Head 패킷임
    //{

    //}
    if (copy_ctx->IsAllStored())
    {
        
        /*outsize = copy_ctx->GetSize();

        
        outBuffer->reserve(outsize);
        
        */
        //  BYTE* newbuffer = new BYTE[outsize];
       // memcpy(outBuffer.data(), &copy_ctx._packetHeader, sizeof(PacketHeader));
      //  memcpy(outBuffer.data(), copy_ctx->_storedBuffer.data(), copy_ctx->GetSize());
        // outBuffer = newbuffer;

        uint32 Sn = copy_ctx->_fragmentSeqNums.top();
        copy_ctx->_fragmentSeqNums.pop();
        
        copy_ctx->SetPrimID(Sn);
        _orderedCtxs[Sn] = copy_ctx;

        while (!copy_ctx->_fragmentSeqNums.empty())
        {
            Sn = copy_ctx->_fragmentSeqNums.top();
            copy_ctx->_fragmentSeqNums.pop();

            PushPassSN(Sn);
            //_fragmentPassSNs.push(Sn); //Pass Key Push
            
            /*

            copy_ctx->SetPrimID(Sn);
            _orderedCtxs[Sn] = make_shared<FragmentContext>(;*/
        }
        _fragCtxs.erase(FgHeader->primID);

     
       
    }
    return IsThereReadyPacket(seqNum, outReadyQueue);
    


    //if (copy_ctx.IsAllStored())
    //{
    //    outsize = copy_ctx.GetSize();

    //    
    //    outBuffer.reserve(outsize);
    //    
    //    
    //    //  BYTE* newbuffer = new BYTE[outsize];
    //   // memcpy(outBuffer.data(), &copy_ctx._packetHeader, sizeof(PacketHeader));
    //    memcpy(outBuffer.data(), copy_ctx._storedBuffer.data(), copy_ctx.GetSize());
    //    // outBuffer = newbuffer;

    //    _fragCtxs.erase(FgHeader->primID);
    //    return true;
    //}

    //return false;
    //
}

//bool Ordered_FG_Manager::PopReadyPacket(OUT std::vector<BYTE>& _outv)
//{
//    WRITE_LOCK;
//    if (_readyPacket.empty())
//        return false;
//    _outv = std::move(_readyPacket.front());
//    _readyPacket.pop();
//
//    return true;
//}
//
//void Ordered_FG_Manager::PushReadyPacket(const std::vector<BYTE>& _packet)
//{
//    WRITE_LOCK;
//    _readyPacket.push(std::move(_packet));
//}

bool Ordered_FG_Manager::IsThereReadyPacket(uint32 expectedseqNum, OUT queue<shared_ptr< FragmentContext>>& _queue)
{
    bool isexist = false;
    
    //int32 popStartSeqNum = _expectedSeqNum;
    do
    {
        //cout << "IsThereReadyPacket" << endl;
        if (!_fragmentPassSNs.empty() && TryPassSN(_expectedSeqNum))
        {
            //PASS
            _expectedSeqNum++;
           //_fragmentPassSNs.pop();
        //    cout << "!_fragmentPassSNs.empty() && _expectedSeqNum == _fragmentPassSNs.top()" << endl;
            continue;
        }
        auto iter = _orderedCtxs.find(_expectedSeqNum);
        if (iter == _orderedCtxs.end())
        {
      //      cout << "iter == _fragCtxs.end() : " << _expectedSeqNum << endl;
            break;

        }
        shared_ptr< FragmentContext> ctx = iter->second;
        if (ctx->IsAllStored())
        {
            isexist = true;
            _queue.push(ctx);
            
            //  PopReadyPacket(iter->second._storedBuffer);
         //   cout << "ctx->IsAllStored() : " << _expectedSeqNum << endl;
            _expectedSeqNum = ctx->GetNextExpectedSeqNum();

            _orderedCtxs.erase(iter);
            continue;
        }
        else
        {
           
            break;
        }
    } while (true);

    return isexist;
}
