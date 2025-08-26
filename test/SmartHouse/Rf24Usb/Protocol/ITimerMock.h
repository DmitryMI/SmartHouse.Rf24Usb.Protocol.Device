#pragma once

#include <gmock/gmock.h>
#include "SmartHouse/Rf24Usb/Protocol/ITimer.h"

namespace SmartHouse::Rf24Usb::Protocol
{
    class ITimerMock : public ITimer
    {
    public:
        MOCK_METHOD(bool, SetTimer, (std::chrono::milliseconds, ITimer::Elapsed, ITimer::Handle& handle), (override));
        MOCK_METHOD(void, ResetTimer, (ITimer::Handle), (override));
        MOCK_METHOD(std::chrono::milliseconds, GetRemainingTime, (ITimer::Handle), (override));
    };

}