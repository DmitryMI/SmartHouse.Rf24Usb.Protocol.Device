#pragma once

#include <gmock/gmock.h>
#include "SmartHouse/Rf24Usb/Protocol/IRf24.h"

namespace SmartHouse::Rf24Usb::Protocol
{
    class IRf24Mock : public IRf24
    {
    public:
        MOCK_METHOD(bool, Read, (uint8_t& pipe, uint8_t& size, (std::array<uint8_t, 32>& data)), (override));
        MOCK_METHOD(bool, Write, (uint8_t size, (const std::array<uint8_t, 32>& data)), (override));
        MOCK_METHOD(bool, Configure, (const ConfigMessage& config), (override));
        MOCK_METHOD(void, PowerDown, (), (override));
        MOCK_METHOD(void, WhatHappened, (bool& txOk, bool& txFail, bool& rxOk), (override));
    };
}