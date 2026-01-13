#include "pch.h"
#include "TransportControlPlane.h"
#include "ThreadManager.h"

TransportControlPlane GTransportControl;


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


TransportControlPlane::TransportControlPlane()
{
    
}

TransportControlPlane::~TransportControlPlane()
{
    StopThread();
}

bool TransportControlPlane::CheckValidControl(PacketHeader* header)
{
    if (!IsControlPacket(header->ControlFlag))
        return false;
    
   

    HandleControlPacket(header);
   
    return true;
}

void TransportControlPlane::RunThread()
{
    GThreadManager->Launch([this]() {
        brunning = true;
        DoWork();
        });
}

void TransportControlPlane::StopThread()
{
    brunning = false;
}

void TransportControlPlane::OnPushRWind(int32 client_id, int32 add_size)
{
  //  cout << "OnPushRWind 1" << endl;
    HostRef host = GHostManager.GetPlayer(client_id);
    if (!host)
        return;
  //  cout << "OnPushRWind 2" << endl;
    jobworker.PushJob([this, client_id, host, add_size]() {
       // cout << "jobworker.PushJob OnPushRWind" << endl;
        SendBufferRef sendBuffer = MakeControlPacketBuffer(client_id);
        ControlHeader* contheader = reinterpret_cast<ControlHeader*>(sendBuffer->Buffer());
        //  Protocol::S_RUDPACK pkt;
        

        // pkt.set_playerid(p.second->client_Id);
        contheader->rwind.bexsist = true;
        contheader->rwind.rwindsize = add_size;
        // pkt.set_rwindsize();

        host->NoWaitPriortySend(sendBuffer);
        });
}

void TransportControlPlane::HandleControlPacket(PacketHeader* header)
{
    if (header->size != ControlPacketSize)
        return;

    ControlHeader* contHeader = reinterpret_cast<ControlHeader*>(&header[1]);

    HostRef player = GHostManager.GetPlayer(header->client_Id);
    if (contHeader->ack.bexsist == true)
    {
        int32 bhascount = contHeader->ack.bhascount;
        int32 start = contHeader->ack.start;
        int32 count = contHeader->ack.count;

        jobworker.PushJob([player, bhascount, start, count]() {
            
           // cout << "jobworker.PushJob ProcessAcks" << endl;
            if (player == nullptr)
            {
                //cout << "Handle_C_RUDPACK PlayerID : " << pkt.playerid() << " | SequenceStart : " << pkt.start() << endl;
               // cout << "jobworker.PushJob ProcessAcks player == nullptr" << endl;
                return;
            }
            // player->GetDeliveryManager()->SetReceiverRWind(pkt.rwindsize());
            player->GetDeliveryManager()->ProcessAcks(start, count, bhascount);
           
            });
        
    }
    if (contHeader->rwind.bexsist == true)
    {
        player->GetDeliveryManager()->AddReceiverRWind(contHeader->rwind.rwindsize);
      //  cout << "Player ID : " << player->client_Id <<" | contHeader->rwind.bexsist == true : " << contHeader->rwind.rwindsize << endl;
    }
    if (contHeader->rtt.bexsist == true)
    {

    }
    

}

void TransportControlPlane::DoWork()
{
    while (brunning)
    {
        //TODO DO TImer Stuff 
        //uint32 ackpreStart = 0;
        uint32 ackStart = 1;
        uint32 ackCount = 0;
        bool hasCount = false;
        NetAddress netAddr;

        //Lastly Do JobQueue
        if (!jobworker.Execute())
        {
            //TODO Sleep 
        }
        //system("cls");

        //Thread 최적화 필요
        

        //for (auto& p : GHostManager.GetPlayers())
        //{
        //    //auto player = p.second;
        //    memset(&netAddr, 0, sizeof(netAddr));
        //    //this_thread::sleep_for(300ms);
        //   // int32 token = 5;
        //    while (true) //AckRange 비울때까지
        //    {
        //        //token--;

        //        if (p.second->GetDeliveryManager()->WritePendingAcks(ackStart, ackCount, hasCount)) // 보낼 Ack이 쌓였다
        //        {
        //            /*if (ackpreStart == ackStart && ackpreStart > 1)
        //            {
        //                cout << "ackpreStart : " << ackpreStart << endl;
        //                CRASH("ackpreStart == ackStart");
        //            }*/
        //            SendBufferRef sendBuffer = MakeControlPacketBuffer(p.second->client_Id);
        //            ControlHeader* contheader = reinterpret_cast<ControlHeader*>(sendBuffer->Buffer() + sizeof(PacketHeader));
        //          //  Protocol::S_RUDPACK pkt;
        //            contheader->ack.bexsist = true;
        //            contheader->ack.bhascount = hasCount;
        //            contheader->ack.count = ackCount;
        //            contheader->ack.start = ackStart;
        //            
        //           // pkt.set_playerid(p.second->client_Id);
        //            contheader->rwind.bexsist = true;
        //            contheader->rwind.rwindsize = p.second->GetRWind();
        //           // pkt.set_rwindsize();
        //            
        //            p.second->NoWaitPriortySend(sendBuffer);
        //            //GUDP.GetUDPSocket(0)->Send(netAddr, sendBuffer);
        //          //  ackpreStart = ackStart;
        //          //  cout << "Send RUDP ACK  " << endl;
        //        }
        //        else
        //            break;
        //    }
        //    p.second->GetDeliveryManager()->ProcessTimeOutPackets();
        //    //LONGLONG curTick = GetTickCount64();
        //    //if (p.second->curRwindTimeStamp == 0)
        //    //    p.second->curRwindTimeStamp = curTick;
        //    //else
        //    //{
        //    //    if (curTick - p.second->curRwindTimeStamp > 100)
        //    //    {
        //    //        SendBufferRef sendBuffer = MakeControlPacketBuffer(p.second->client_Id);
        //    //        ControlHeader* contheader = reinterpret_cast<ControlHeader*>(sendBuffer->Buffer() + sizeof(PacketHeader));
        //    //        //  Protocol::S_RUDPACK pkt;

        //    //        // pkt.set_playerid(p.second->client_Id);
        //    //        contheader->rwind.bexsist = true;
        //    //        contheader->rwind.rwindsize = p.second->GetRWind();
        //    //        // pkt.set_rwindsize();

        //    //        p.second->NoWaitPriortySend(sendBuffer);

        //    //        p.second->curRwindTimeStamp = curTick;
        //    //    }
        //    //}
        //    //PacketDeliverCondition(static_pointer_cast<Player>(p.second));
        //}
        //

        
    }
}



SendBufferRef TransportControlPlane::MakeControlPacketBuffer(int32 client_id)
{
    SendBufferRef sendBuffer = GSendBufferManager->Open(ControlPacketSize);
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
    header->channel = QoSCore::Channel::URO;
    header->size = ControlPacketSize;
    header->ControlFlag = 1;
    header->client_Id = client_id;
    sendBuffer->Close(ControlPacketSize);
    return sendBuffer;
}

