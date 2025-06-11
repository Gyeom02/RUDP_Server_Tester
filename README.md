RUDP 기능인 기본적인 패킷순서보장, 패킷손실보장, QoS(우선순위) 지원하는 소켓이다

QoS에서 우선순위 패킷은 High, Medium, Low 패킷으로 나눠지며 
Queue에서 고유 클라이언트의 독점을 방지하기 위해 Sharded 형식, TokenBucket을 이용

기본 UDP 소켓통신
![0 1ms_No_RUDP](https://github.com/user-attachments/assets/6ded6cf2-ca59-4eb5-ab5d-9320a27d7e63)

RUDP 소켓통신
![0 1ms_RUDP](https://github.com/user-attachments/assets/0d1989c0-c159-4678-8fd2-6a325cfa9cf1)


https://github.com/user-attachments/assets/70e8587b-111a-4dc5-8394-fad01d27100e

