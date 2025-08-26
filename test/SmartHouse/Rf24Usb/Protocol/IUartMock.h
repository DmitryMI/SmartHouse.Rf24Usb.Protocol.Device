#pragma once

#include <gmock/gmock.h>
#include "SmartHouse/Rf24Usb/Protocol/IUart.h"

namespace SmartHouse::Rf24Usb::Protocol
{
    class IUartMock : public IUart
    {
    public:
        MOCK_METHOD(bool, Read, (uint8_t&), (override));
        MOCK_METHOD(void, Write, (const uint8_t* data, size_t len), (override));
        MOCK_METHOD(void, Flush, (), (override));
    };
}