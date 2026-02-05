#include "pch.h"
#include "RTTManager.h"
#include <algorithm>

RTTManager::RTTManager()
    : _baseRTT(0.0), _oldSRTT(0.0), _rttGradient(0.0f)
{
#ifdef _DEBUG
    if (!_paceRateBytePerSec.is_lock_free() || !_paceSendBudget.is_lock_free())
        CRASH("!_paceRateBytePerSec.is_lock_free() || !_paceSendBudget.is_lock_free()");
#endif
    _paceRateBytePerSec.store(_InitSendRate);
    _paceSendBudget.store(_paceRateBytePerSec.load());
}
RTTManager::~RTTManager()
{

}


uint64 RTTManager::GetRTT(uint64 time_stamp)
{
#ifdef _DEBUG
    if (time_stamp <= 0)
        CRASH("rtt_stamp <= 0");
#endif
    uint64 now = UTime::GetNow();
    if (now < time_stamp)
        return 0;
    return now - time_stamp;
}

uint64 RTTManager::GetRTT_milsec(uint64 time_stamp)
{
    return GetRTT(time_stamp) / 1000;
}

void RTTManager::HandleACK(double rtt)
{


    //double rtt = GetRTT(time_stamp); // microseconds rtt

    if (rtt > Reasonable_RTT_Us)
        return;

    _recentRttsQueue.push_back({ UTime::GetNow(), rtt });
    

    if (_baseRTT == 0.0 && _oldSRTT == 0.0)
    {
        _baseRTT = rtt;
        _oldSRTT = rtt;
        _rttVar.store(rtt * 0.5);
    }

    else
    {
        double err = rtt - _oldSRTT;
        double prevSRTT = (1 - alpha) * _oldSRTT + alpha * rtt;
        _rttGradient = prevSRTT - _oldSRTT; // 양수 -> RTT가 점점 증가하고 있다, 음수 -> RTT가 감소하고있다.
        _oldSRTT = prevSRTT;

        double oldrttVar = _rttVar.load();
        _rttVar.exchange(oldrttVar + (fabs(err) - oldrttVar) * 0.25);
       
    }
   

    UpdatePaceRate(rtt);
    
}

void RTTManager::UpdatePaceRate(double newRTT)
{
    uint8 Flag = NULL_FLAG;


    double _queueDelay = _oldSRTT - _baseRTT;

    if (_queueDelay < 0)
        _queueDelay = 0;

    UpdateRttQueue(_queueDelay);

    double sendRate = 0.0f;
    if (_queueDelay > CONGESTED)
    {
       // Flag = CONGEST_FLAG;
        sendRate = _paceRateBytePerSec.load() * Congest_Rate;
    }
    else if (_queueDelay > BUILDUP)
    {
       // Flag = CONGEST_FLAG;
        sendRate = _paceRateBytePerSec.load() * BuildUp_Rate;
    }
    else
    {
        sendRate = _paceRateBytePerSec.load() * Recover_Rate;
    }

    _paceRateBytePerSec.exchange(std::clamp(sendRate, _minSendRate, _maxSendRate));

//#ifdef _DEBUG
//    _debugQueueDelay = _queueDelay;
//#endif
 /*   else if (DRAINING <= _queueDelay && _queueDelay <= BUILDUP)
    {
        Flag = IDLE_FLAG;
    }
    else if (_queueDelay < DRAINING)
    {
        Flag = IDLE_FLAG;
        _baseRTT = _baseRTT < newRTT ? newRTT : _baseRTT;
    }*/

  
    //  auto oldRttGradient = _rttGradient;
    

}


void RTTManager::UpdateCongestBudget()
{
    uint64 now = UTime::GetNow();

    if (_lastUpdateBudgetUs == 0)
    {
        _lastUpdateBudgetUs = now;
        return;
    }

    double deltaSec = (now - _lastUpdateBudgetUs) / 1'000'000.0; // (Us / 1'000'000.0) = microsecond를 초단위로 바꾸는 행위
    _lastUpdateBudgetUs = now;

    double expectedBudget = _paceSendBudget.load() + (_paceRateBytePerSec * deltaSec);
   // _paceSendBudget.exchange();

    double maxBurst = _paceRateBytePerSec * 0.25; // 250ms
    if (expectedBudget > maxBurst) // Burst 제어
        _paceSendBudget.exchange(maxBurst);
    else
        _paceSendBudget.exchange(expectedBudget);
}

void RTTManager::UpdateRttQueue(double queueDelay)
{
    uint64 now = UTime::GetNow();
    while (true)
    {
        if (_recentRttsQueue.empty())
            return;
        auto& _front = _recentRttsQueue.front();
        if (_front._when_made - now > RttQueueValidTime)
            _recentRttsQueue.pop_front();
        else break;
    }

    if (queueDelay < _baseRTT * 0.1) //거의 Idle한 상태일때만
    {
        _baseRTT = std::min_element(_recentRttsQueue.begin(), _recentRttsQueue.end(), [](auto& a, auto& b) { return a._rtt_us < b._rtt_us; })->_rtt_us;
    }
    
    while (kMaxRttSamples < _recentRttsQueue.size())
        _recentRttsQueue.pop_back();
}

double RTTManager::GetResendDelay(int32 retransCount)
{
    double baseDelay =  _oldSRTT + _rttVar.load() * 3.0;
    const double kMinDelayUs = 2'000.0;   // 2ms

    baseDelay = baseDelay > kMinDelayUs ? baseDelay : kMinDelayUs; //최소 하한
    double backoff = 1.0 + retransCount * 0.5;

    double resendDelay = baseDelay * backoff;

    const double kMaxDelayUs = 500'000.0; // 500ms
    resendDelay = resendDelay > kMaxDelayUs ? kMaxDelayUs : resendDelay; // 최대 상한

    return resendDelay;
}

bool RTTManager::ValidBudget(int32 packetSize)
{
    if (_paceSendBudget.load() - double(packetSize) < 0)
    {
        //cout << "ValidBudget Fail" << endl;
        return false;
    }
    return true;
}

void RTTManager::UseBudget(int32 packetSize)
{
#ifdef _DEBUG
    ASSERT_CRASH(packetSize > 0);
    ASSERT_CRASH(packetSize <= _paceSendBudget.load());
#endif
    double prev = _paceSendBudget.load();
    _paceSendBudget.compare_exchange_strong(prev, prev - packetSize);
}

