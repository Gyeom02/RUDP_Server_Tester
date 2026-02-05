#include "pch.h"
#include "TransportControl.h"
#include "ThreadManager.h"

TransportControl GTransportControl;


void ControlJobWorker::PushJob(CallbackType&& callback)
{
    WRITE_LOCK;
    //cout << "ControlJobWorker::PushJob()" << endl;
    _jobs.push(MakeShared<Job>(std::move(callback)));
    
}

bool ControlJobWorker::Execute()
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
    _jobWorker.PushJob([this, client_id, host, add_size, total_recovered_size]() {
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
        if (start > 55338981)
        {
            cout << "A" << endl;
        }
        _jobWorker.PushJob([player, bhascount, start, count, curExpectedSN]() {
            
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
        
        _jobWorker.PushJob([player, rtt]() { player->HandleACK(rtt); });
       
    }
    

}

void TransportControl::DoWork()
{
    HostRef ackReadyHost;
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
            _lazyAssist._jobCv.wait_until(_lock, _lazyAssist._nextTick, [&]() { return !_jobWorker.IsEmpty() || !EmptyReadyAckQueue() || !brunning.load(); });
            //Lastly Do JobQueue
            if (brunning.load() == false)
                return;
            if (!EmptyReadyAckQueue())
            {
                
                ackReadyHost = PopHostAckReady();
            }
            
        }

        if (!_jobWorker.IsEmpty())
        {
            _jobWorker.Execute();
        }

        if (ackReadyHost)
        {
           
            HandleHostReadyAck(ackReadyHost);
            ackReadyHost = nullptr;
        }

      //  cout << "lazyAssist._jobCv.wait_until" << endl;
        //system("cls");

        //Thread 최적화 필요
        
       
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
           // p.second->GetDeliveryManager()->ProcessTimeOutPackets();

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
            SendBufferRef sendBuffer = MakeAckControlPacket(host->client_Id, hasCount, ackStart, ackCount, host->GetDeliveryManager()->GetExpectedSeqNum(), rtt_stamp);

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

void TransportControl::PeriodicRwindSync(HostRef host, int32 rwindsize, uint32 total_recovered_size)
{
   // HostRef host = GHostManager.GetPlayer(client_id);
   // cout << "PeriodicRwindSync" << endl;
    int32 client_id = host->client_Id;
    if (!host)
        return;
    //  cout << "OnPushRWind 2" << endl;
    _jobWorker.PushJob([this, client_id, host, rwindsize, total_recovered_size]() {
        //   cout << "jobworker.PushJob OnPushRWind" << endl;
        SendBufferRef sendBuffer = MakeADRwindControlPacket(client_id, rwindsize, total_recovered_size);

        // pkt.set_rwindsize();

        host->ControlSend(sendBuffer);
        });
    
}

