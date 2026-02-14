#include "pch.h"
#include "TransportControl.h"
#include "ThreadManager.h"

TransportControl GTransportControl;


void ControlJobs::PushJob(CallbackType&& callback)
{
    WRITE_LOCK;
    //cout << "ControlJobWorker::PushJob()" << endl;
    _jobs.push(MakeShared<Job>(std::move(callback)));
    
    
}

bool ControlJobs::Execute()
{
    JobRef job;
 
    {
        WRITE_LOCK;
        if (_jobs.empty())
            return false;
        job = _jobs.front();
        _jobs.pop();
    }
   // cout << "ControlJobWorker::Execute()" << endl;
    job->Execute();
    return true;
}

bool ControlJobs::CheckHostControlJobEmpty()
{
    WRITE_LOCK;
    if (IsEmpty()) //비어있음을 확인
    {
        bInsertReadyQueue.exchange(false);
        return true;
    }
    return false;
  
}


TransportControl::TransportControl(uint32 tickms)
    : _lazyAssist(tickms)

{
    
}

TransportControl::~TransportControl()
{
    StopThread();
}

bool TransportControl::CheckValidControl(PacketHeader* header)
{
    if (!IsControlPacket(header->controlflag))
        return false;
    
   

    HandleControlPacket(header);
   
    return true;
}

void TransportControl::RunThread()
{
    GThreadManager->Launch([this]() {
        brunning.store(true);
        _lazyAssist._nextTick = std::chrono::steady_clock::now() + std::chrono::microseconds(_lazyAssist._tickMs);
        DoWork();
        });
}

void TransportControl::StopThread()
{
    brunning.store(false);
    _lazyAssist._jobCv.notify_one();
}

void TransportControl::OnPushRWind(int32 client_id, int32 add_size, uint32 total_recovered_size)
{
  //  cout << "OnPushRWind 1" << endl;
    HostRef host = GHostManager.GetPlayer(client_id);
    if (!host)
        return;
  //  cout << "OnPushRWind 2" << endl;
    host->PushControlJob([this, client_id, host, add_size, total_recovered_size]() {
     //   cout << "jobworker.PushJob OnPushRWind" << endl;
        SendBufferRef sendBuffer = MakeRecoverRwindControlPacket(client_id, add_size, total_recovered_size);
       
        // pkt.set_rwindsize();

        host->ControlSend(sendBuffer);
        });
}

bool TransportControl::EmptyReadyAckQueue()
{
    READ_LOCK; 
    return _readyAckHostQueue.empty();
}

void TransportControl::PushHostAckReady(HostRef host)
{
    if (!host) 
        return;  
    WRITE_LOCK; 
    _readyAckHostQueue.push(host);
}

HostRef TransportControl::PopHostAckReady()
{
    HostRef popHost = nullptr;
   
    {
        WRITE_LOCK;
        if (EmptyReadyAckQueue())
            return popHost;
        popHost = _readyAckHostQueue.front();
        _readyAckHostQueue.pop();
    }
    return popHost;
}

bool TransportControl::EmptyReadyControlJobQueue()
{
    bool bempty;
    {
        READ_LOCK_IDX(1);
        bempty = _readyControlJobHostQueue.empty();
    }
    return bempty;
}

void TransportControl::PushHostControlJobReady(HostRef host)
{
    if (!host)
        return;
    WRITE_LOCK_IDX(1);
    _readyControlJobHostQueue.push(host);
}

HostRef TransportControl::PopHostControlJobReady()
{

    HostRef popHost = nullptr;

    {
        WRITE_LOCK_IDX(1);
        if (EmptyReadyControlJobQueue())
            return popHost;
        popHost = _readyControlJobHostQueue.front();
        _readyControlJobHostQueue.pop();
    }

    return popHost;
    
}

void TransportControl::PushRTOQueue(const Packet_RTO_State& rhs)
{
    WRITE_LOCK_IDX(2);
    _rtoMinQueue.push(rhs);
}

void TransportControl::PopRTOQueue()
{
    WRITE_LOCK_IDX(2);
    _rtoMinQueue.pop();
}

void TransportControl::HandleControlPacket(PacketHeader* header)
{
    if (header->size != ControlPacketSize)
        return;

    ControlHeader* contHeader = reinterpret_cast<ControlHeader*>(&header[1]);

    HostRef player = GHostManager.GetPlayer(header->client_Id);

    
    if (contHeader->ack.bexsist == true)
    {
       // cout << "contHeader->ack.bexsist == true" << endl;
        int32 bhascount = contHeader->ack.bhascount;
        uint32 start = contHeader->ack.start;
        int32 count = contHeader->ack.count;
        uint32 curExpectedSN = contHeader->ack.curExpectedSN;
#ifdef _DEBUG
        if (start > 55338981)
        {
            CRASH("start > 55338981");
        }
#endif
        player->PushControlJob([player, bhascount, start, count, curExpectedSN]() {
            
            //cout << "jobworker.PushJob ProcessAcks" << endl;
            if (player == nullptr)
            {
                //cout << "Handle_C_RUDPACK PlayerID : " << pkt.playerid() << " | SequenceStart : " << pkt.start() << endl;
               // cout << "jobworker.PushJob ProcessAcks player == nullptr" << endl;
                return;
            }
            // player->GetDeliveryManager()->SetReceiverRWind(pkt.rwindsize());
            player->GetDeliveryManager()->ProcessAcks(start, count, bhascount, curExpectedSN);
            ///player->GetDeliveryManager()->UpdateExpectedAckSN(curExpectedSN);
            });
        
    }
    if (contHeader->rwind.bexsist == true)
    {
        uint32 curTotalRWind = player->GetDeliveryManager()->GetTotal_Send_RWind();
        if (contHeader->Type == ControlType::RECOVER_RWIND)
        {
            if (contHeader->rwind.total_recovered_size <= curTotalRWind)
                return; //RECOVER_RWIND Type 패킷(같거나 또는 더 큰 total_recovered_size를 갖은)이 먼저 옴
            player->GetDeliveryManager()->AddReceiverRWind(contHeader->rwind.rwindsize);
            player->GetDeliveryManager()->SetTotal_Send_RWind(contHeader->rwind.total_recovered_size);
           // cout << "Recover RWind : " << player->GetDeliveryManager()->GetReceiverRWind() << endl;
        }
        else if(contHeader->Type == ControlType::AD_RWIND)
        {
           // cout << "contHeader->Type == ControlType::AD_RWIND : " << player->client_Id << " | rwindsize : " << contHeader->rwind.rwindsize << endl;
            if (contHeader->rwind.total_recovered_size <= curTotalRWind)
            {
               // cout << "contHeader->rwind.total_recovered_size <= curTotalRWind" << endl;
                return; //RECOVER_RWIND Type 패킷(같거나 또는 더 큰 total_recovered_size를 갖은)이 먼저 옴
            }
           
            uint32 deltaSize = contHeader->rwind.total_recovered_size - curTotalRWind; // 안 적용된 recovered Rwind size
           // cout << "deltaSize : " << deltaSize << endl;
           // cout << "curTotalRWind : " << curTotalRWind << endl;
            player->GetDeliveryManager()->AddReceiverRWind(deltaSize);
            player->GetDeliveryManager()->SetTotal_Send_RWind(contHeader->rwind.total_recovered_size);
           
            if (player->GetDeliveryManager()->GetReceiverRWind() > contHeader->rwind.rwindsize)
                player->GetDeliveryManager()->StoreReceiverRWind(contHeader->rwind.rwindsize);
        }
       // cout << "Player ID : " << player->client_Id <<" | contHeader->rwind.bexsist == true : " << contHeader->rwind.rwindsize << endl;
    }
    if (contHeader->rtt.bexsist == true)
    {
        double rtt = RTTManager::GetRTT(contHeader->rtt.sent_timestamp);
        
        player->PushControlJob([player, rtt]() { player->HandleACK(rtt); });
       
    }
    

}

void TransportControl::DoWork() // Only One Thread Has to Run this Work Function(SPSC Queue Using)
{
    HostRef ackReadyHost;
    HostRef ControlJobReadyHost;
    while (brunning.load())
    {
        
       /* auto now = chrono::steady_clock::now();
        if (now >= _nextTick)
        {

        }*/
        //TODO DO TImer Stuff 
        //uint32 ackpreStart = 0;
        {
            _lazyAssist.SetNextTickFromNow();

            unique_lock<mutex> _lock(_lazyAssist._jobMutex);
            _lazyAssist._jobCv.wait_until(_lock, _lazyAssist._nextTick, [&]() { return !EmptyReadyControlJobQueue() || !EmptyReadyAckQueue() || !brunning.load(); });
            //Lastly Do JobQueue
            if (brunning.load() == false)
                return;

            if (!EmptyReadyControlJobQueue())
            {
                ControlJobReadyHost = PopHostControlJobReady();
            }
            if (!EmptyReadyAckQueue())
            {
                
                ackReadyHost = PopHostAckReady();
            }
           
        }

        if (ControlJobReadyHost)
        {
            HandleHostReadyControlJob(ControlJobReadyHost);
            ControlJobReadyHost = nullptr;
            //_controlJobWorker.Execute();
        }

        if (ackReadyHost)
        {
           
            HandleHostReadyAck(ackReadyHost);
            ackReadyHost = nullptr;
        }

      //  cout << "lazyAssist._jobCv.wait_until" << endl;
        //system("cls");

        //Thread 최적화 필요
        CheckTimeOutPacket();
       
        for (auto& p : GHostManager.GetPlayers())
        {
            //auto player = p.second;
           // memset(&netAddr, 0, sizeof(netAddr));
           // //this_thread::sleep_for(300ms);
           //// int32 token = 5;
           // while (true) //AckRange 비울때까지
           // {
           //     //token--;

           //     if (p.second->GetDeliveryManager()->WritePendingAcks(ackStart, ackCount, hasCount)) // 보낼 Ack이 쌓였다
           //     {
           //         /*if (ackpreStart == ackStart && ackpreStart > 1)
           //         {
           //             cout << "ackpreStart : " << ackpreStart << endl;
           //             CRASH("ackpreStart == ackStart");
           //         }*/
           //         SendBufferRef sendBuffer = MakeAckControlPacket(p.second->client_Id, hasCount, ackStart, ackCount);

           //         p.second->NoWaitPriortySend(sendBuffer);
           //         //GUDP.GetUDPSocket(0)->Send(netAddr, sendBuffer);
           //       //  ackpreStart = ackStart;
           //       //  cout << "Send RUDP ACK  " << endl;
           //     }
           //     else
           //         break;
           // }
             //p.second->GetDeliveryManager()->ProcessTimeOutPackets();

            LONGLONG curTick = GetTickCount64();
            if (p.second->curRwindTimeStamp == 0)
                p.second->curRwindTimeStamp = curTick;
            else
            {
                if (curTick - p.second->curRwindTimeStamp > AD_RWIND_PERIOD)
                {
                    PeriodicRwindSync(p.second, p.second->GetDeliveryManager()->GetRWind(), p.second->GetDeliveryManager()->GetTotal_Recv_RWind());
                 

                    p.second->curRwindTimeStamp = curTick;
                }
            }
            //PacketDeliverCondition(static_pointer_cast<Player>(p.second));
        }
        

        
    }
}

SendBufferRef TransportControl::MakeAckControlPacket(int32 client_id,  int32 bhascount, uint32 start, int32 count, uint32 curExpectedSN, uint64 rtt_timestamp)
{
    SendBufferRef sendBuffer = MakeControlPacketBuffer(client_id);
    ControlHeader* contheader = InitControlHeader(sendBuffer->Buffer());
    //  Protocol::S_RUDPACK pkt;
    contheader->Type = ControlType::ACK;
    contheader->ack.bexsist = true;
    contheader->ack.bhascount = bhascount;
    contheader->ack.start = start;
    contheader->ack.count = count;
    contheader->ack.curExpectedSN = curExpectedSN;

    if (rtt_timestamp > 0)
    {
        contheader->rtt.bexsist = true;
        contheader->rtt.sent_timestamp = rtt_timestamp;
    }
    return sendBuffer;
}

SendBufferRef TransportControl::MakeRecoverRwindControlPacket(int32 client_id, int32 rwindsize, uint32 total_recovered_size)
{
    SendBufferRef sendBuffer = MakeControlPacketBuffer(client_id);
    ControlHeader* contheader = InitControlHeader(sendBuffer->Buffer());
    //  Protocol::S_RUDPACK pkt;


    // pkt.set_playerid(p.second->client_Id);
    contheader->Type = ControlType::RECOVER_RWIND;
    contheader->rwind.bexsist = true;
    contheader->rwind.rwindsize = rwindsize;
    contheader->rwind.total_recovered_size = total_recovered_size;
    return sendBuffer;
}

SendBufferRef TransportControl::MakeADRwindControlPacket(int32 client_id, int32 rwindsize, uint32 total_recovered_size)
{
    SendBufferRef sendBuffer = MakeControlPacketBuffer(client_id);
    ControlHeader* contheader = InitControlHeader(sendBuffer->Buffer());
    //  Protocol::S_RUDPACK pkt;


    // pkt.set_playerid(p.second->client_Id);
    contheader->Type = ControlType::AD_RWIND;
    contheader->rwind.bexsist = true;
    contheader->rwind.rwindsize = rwindsize;
    contheader->rwind.total_recovered_size = total_recovered_size;
    return sendBuffer;
}



SendBufferRef TransportControl::MakeControlPacketBuffer(int32 client_id)
{
    SendBufferRef sendBuffer = GSendBufferManager->Open(ControlPacketSize);
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
    header->channel = QoS::Channel::URO;
    header->size = ControlPacketSize;
    header->controlflag = 1;
    header->client_Id = client_id;
    header->bFragment = 0;
    sendBuffer->Close(ControlPacketSize);
    return sendBuffer;
}

ControlHeader* TransportControl::InitControlHeader(BYTE* buffer)
{
    
    ControlHeader* contHeader = reinterpret_cast<ControlHeader*>(buffer  + sizeof(PacketHeader));

    contHeader->ack.bexsist = false;
    contHeader->ack.bhascount = 0;
    contHeader->ack.start = 0;
    contHeader->ack.count = -1;
    contHeader->ack.curExpectedSN = 0;

    contHeader->rwind.bexsist = false;
    contHeader->rwind.rwindsize = -1;
    contHeader->rwind.total_recovered_size = -1;

    contHeader->rtt.bexsist = false;
    contHeader->rtt.sent_timestamp = 0;
    
   
    return contHeader;
}

void TransportControl::HandleHostReadyAck(HostRef host)
{
   
    //TODO Send Ack
    uint32 ackStart = 1;
    int32 ackCount = 0;
    bool hasCount = false;
    uint64 rtt_stamp = 0;

    NetAddress netAddr;

    int32 token = ACKTOKEN;

    bool bAckEmpty = false;
    while (token--)
    {
        if (host->GetDeliveryManager()->WritePendingAcks(ackStart, ackCount, hasCount, rtt_stamp)) // 보낼 Ack이 쌓였다
        {
            /*if (ackpreStart == ackStart && ackpreStart > 1)
            {
                cout << "ackpreStart : " << ackpreStart << endl;
                CRASH("ackpreStart == ackStart");
            }*/
            SendBufferRef sendBuffer = MakeAckControlPacket(host->client_Id, hasCount, ackStart, ackCount, host->GetDeliveryManager()->GetRWindExpectedSeqNum(), rtt_stamp);

            host->ControlSend(sendBuffer);
            //GUDP.GetUDPSocket(0)->Send(netAddr, sendBuffer);
          //  ackpreStart = ackStart;
          //  cout << "Send RUDP ACK  " << endl;
        }
        else
        {
            bAckEmpty = true;
            break;
        }
    }

    
    if (bAckEmpty && host->GetDeliveryManager()->CheckHostAckEmpty() /*정밀 Check*/)
    {
        
        return; //다 보냄 
    }
    // 해당 호스트의 Ack을 다 못 보냄 다시 AckReadyQueue에 넣어야함
    
    PushHostAckReady(host);
    GetLazyAssist()._jobCv.notify_one();
}

void TransportControl::HandleHostReadyControlJob(HostRef host)
{
    int32 token = CONTROLJOB_TOKEN;

    bool bAckEmpty = false;
    while (token--)
    {
        if (!host->GetControlJobs().Execute()) // Control Job Queue Empty
        {
            bAckEmpty = true;
            break;
        }
       
    }


   

    if (bAckEmpty && host->GetControlJobs().CheckHostControlJobEmpty()/*정밀 Check*/)
    {

        return; //다 보냄 
    }
    // 해당 호스트의 Ack을 다 못 보냄 다시 AckReadyQueue에 넣어야함

    PushHostControlJobReady(host);
    GetLazyAssist()._jobCv.notify_one();
}

void TransportControl::PeriodicRwindSync(HostRef host, int32 rwindsize, uint32 total_recovered_size)
{
   // HostRef host = GHostManager.GetPlayer(client_id);
   // cout << "PeriodicRwindSync" << endl;
    int32 client_id = host->client_Id;
    if (!host)
        return;
    //  cout << "OnPushRWind 2" << endl;
    host->PushControlJob([this, client_id, host, rwindsize, total_recovered_size]() {
        //   cout << "jobworker.PushJob OnPushRWind" << endl;
        SendBufferRef sendBuffer = MakeADRwindControlPacket(client_id, rwindsize, total_recovered_size);

        // pkt.set_rwindsize();

        host->ControlSend(sendBuffer);
        });
    
}

void TransportControl::CheckTimeOutPacket()
{
    
    while (true)
    {

        Packet_RTO_State state;
        {
            WRITE_LOCK_IDX(2);
            if (_rtoMinQueue.empty())
                return;
            state = _rtoMinQueue.top();
            _rtoMinQueue.pop();
            if (state.rto_us > UTime::GetNow()) // 아직 타임아웃 아님
                return;
        }
        HostRef host = state.inflightPacket->GetOwner();
        if (!host)
        {
            
            continue;
        }
        
        //uint32 nextInFlightPacketSN;
        
        //int32 handleFlag = -1; // 1 = success | 2 = fail

        //WRITE_LOCK_IDX(InFlightPacket_LOCK);
       // if (mInFlightPacketsSN.Empty())
          //  return;


        //nextInFlightPacketSN = state.sn;

        InFlightPacketPtr& nextInFlightPacket = state.inflightPacket;
        //cout << "1" << endl;
        if (nextInFlightPacket->IsGotAck()) // 이미 Ack 처리한 패킷임
        {
            //PopRTOQueue();
            continue;
        }

        //uint64 now = UTime::GetNow();

        

            //if (nextInFlightPacket->GetTransmissionData()->IsGotAck()) // Ack Packet을 받은 패킷임
            //{
            //	mInFlightPackets.pop_front();
            //}

        //PacketHeader* header = reinterpret_cast<PacketHeader*>(nextInFlightPacket->GetTransmissionData()->Buffer());
        DeliveryManagerRef deliveryManager = host->GetDeliveryManager();
        uint32 inflightSN = nextInFlightPacket->GetSequenceNumber().GetSN();
        if (inflightSN < deliveryManager->GetExpectedAckSN()) //_curExpectedAckSN보다 작다는건 이미 처리된 SN을 가진 패킷임으로 성공처리함(그저 Ack을 못받았을 뿐)
        {
            //cout << "2" << endl;
            
           // PopRTOQueue();

            deliveryManager->HandleAck(inflightSN);
            deliveryManager->HandlePacketDeliverySuccess(nextInFlightPacket);

            //continue;
        }
        else
        {
            /*if (!nextInFlightPacket->Check_CoolTime_ReSend())
                return;*/
            //cout << "3" << endl;

            //cout << "4" << endl;
            deliveryManager->AddTimeOutCount();

            deliveryManager->HandlePacketDeliveryFailure(nextInFlightPacket);
            //PacketHeader* header = reinterpret_cast<PacketHeader*>(nextInFlightPacket->GetTransmissionData()->Buffer());
        //	cout << " TimeOutPacket ID : " << header->id << " | SN : " << header->sn << endl;

            //mInFlightPacketsSN.Pop();
            return;
        }

        
       

    }
}

