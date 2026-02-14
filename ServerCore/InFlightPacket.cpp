#include "pch.h"
#include "InFlightPacket.h"

void InFlightPacket::HandleDeliveryFailure(DeliveryManagerRef deliveryManager)
{
	SendBufferRef sendBuffer = GetTransmissionData();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	int32 prevRetransNum = header->retransnum;
	

	header->priority = QoS::Priority::RESEND; // 재전송이므로 제일 높은 우선순위로 변경
	header->retransnum = prevRetransNum + 1;

	HostRef owner = GetOwner();
	if (owner)
	{
		owner->Send(sendBuffer);
	}
	//if (header->retransnum > 3) // 3번의 재전송 기회를 다씀
	//{
	//	header->retransnum = 0;
	//	//Pass
	//}
	//else // 아직 재전송 할 수 있음
	//{
	//	header->priority = QoS::Priority::RESEND; // 재전송이므로 제일 높은 우선순위로 변경
	//	//	cout << "ReSend" << endl;
	//	HostRef owner = GetOwner();
	//	if (owner)
	//	{
	//		//deliveryManager->WriteSeqeuenceNumber(sendBuffer);
	//		//Send();
	//		owner->Send(sendBuffer);
	//	}
	//}
}

void InFlightPacket::HandleDeliverySuccess(DeliveryManagerRef deliveryManager)
{
	SendBufferRef sendBuffer = GetTransmissionData();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	if (header->retransnum >= 1) // 재전송패킷 성공했다는 뜻이다
	{
		//cout << "header->retransnum : " << header->retransnum << endl;
		//int32 n = GDeliveryManager->GetSuccessReSendPacketNum();
		//cout << "header->retransnum : " << header->retransnum << endl;

		deliveryManager->mSuccessReSendPacketNum += header->retransnum; // 1은 처음에 보낸 것을 의미
		header->retransnum = 0;
	}
	//
}
void InFlightPacket::InitSendTime()
{
	uint64 now = UTime::GetNow();
	_time_first_send.store(now);
	_time_recent_send.store(now);

}
uint64 InFlightPacket::GetRecentSendTime()
{
	return _time_recent_send.load();
}
void InFlightPacket::UpdateRecentSendTime()
{
	_time_recent_send.exchange(UTime::GetNow());
}
bool InFlightPacket::Check_CoolTime_ReSend()
{
	if (_time_recent_resend == 0)
	{
		_time_recent_resend = UTime::GetNow();
	}
	else
	{
		uint64 now = UTime::GetNow();
		double ResendDelay = _owner.lock()->GetRTTManager().GetResendDelay(reinterpret_cast<PacketHeader*>(GetTransmissionData()->Buffer())->retransnum);
		if (double(now - _time_recent_resend.load()) <= ResendDelay)
		{
			return false;
		}
		_time_recent_resend = now;
	}
	return true;
}
//
//int32 InFlightPacket::Send()
//{
//
//	SOCKADDR_IN netaddr = GetOwner()->netAddress.GetSockAddr();
//	//cout << netAddr.GetPort()<< endl;
//	//wcout << netAddr.GetIpAddress() << endl;
//	int32 addrLen = sizeof(netaddr);
//	while (true)
//	{
//		if (::sendto(GetOwner()->ownerSocket->GetSocket(), reinterpret_cast<const char*>(_sendBuffer->Buffer()), _sendBuffer->WriteSize(), 0, reinterpret_cast<SOCKADDR*>(&netaddr), addrLen) == SOCKET_ERROR)
//		{
//			if (::WSAGetLastError() == WSAEWOULDBLOCK)
//				continue;
//			break;
//		}
//		else break;
//	}
//	return _sendBuffer->WriteSize();
//	
//
//
//}
