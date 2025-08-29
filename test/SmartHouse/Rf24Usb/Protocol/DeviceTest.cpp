#include "Utils.h"
#include "SmartHouse/Rf24Usb/Protocol/Device.h"
#include "SmartHouse/Rf24Usb/Protocol/Encoder.h"

using namespace testing;

namespace SmartHouse::Rf24Usb::Protocol
{

	TEST(DeviceTest, NormalSequenceTest)
	{
		Encoder<MaxMessageSize> encoder;
		constexpr auto configMessage = CreateConfigMessage();
		auto serializedConfig = encoder.Serialize(configMessage);
		constexpr auto hostPayloadMessage1 = CreateHostPayloadMessage(10, 0);
		auto hostPayloadSerialized1 = encoder.Serialize(hostPayloadMessage1);
		constexpr auto hostPayloadMessage2 = CreateHostPayloadMessage(32, 13);
		auto hostPayloadSerialized2 = encoder.Serialize(hostPayloadMessage2);
		constexpr PayloadMessage devicePayloadMessage{ 3, 2, {0xAB, 0xCD} };

		std::queue<uint8_t> uartHostToDevice;
		std::queue<uint8_t> uartDeviceToHost;

		IUartMock uart;
		IRf24Mock rf24;
		ITimerMock timer;

		ExpectUartReadWrite(uart, uartHostToDevice, uartDeviceToHost);

		EXPECT_CALL(rf24, Read(_, _, _))
			.WillOnce([&](uint8_t& pipe, uint8_t& size, std::array<uint8_t, 32>& data) {
			pipe = devicePayloadMessage.m_Pipe;
			size = devicePayloadMessage.m_Size;
			data = devicePayloadMessage.m_Data;
			return true;
				})
			.WillOnce([&](uint8_t& pipe, uint8_t& size, std::array<uint8_t, 32>& data) {
			return false;
				});

		EXPECT_CALL(timer, SetTimer(_, _, _)).WillRepeatedly(Return(true));
		EXPECT_CALL(timer, ResetTimer(_)).Times(AnyNumber());

		EXPECT_CALL(rf24, Configure(configMessage)).WillOnce(Return(true));
		EXPECT_CALL(rf24, WhatHappened(_, _, _)).WillRepeatedly([](bool& txOk, bool& txFail, bool& rxOk) {txOk = false; txFail = false, rxOk = true; });
		EXPECT_CALL(rf24, Write(hostPayloadMessage1.m_Size, hostPayloadMessage1.m_Data)).WillOnce(Return(true));
		EXPECT_CALL(rf24, Write(hostPayloadMessage2.m_Size, hostPayloadMessage2.m_Data)).WillOnce(Return(false));

		Device device(uart, rf24, timer);

		Write(uartHostToDevice, 0, 3, serializedConfig);

		device.OnUartRxInterrupt();
		device.Tick();

		// Uncommend if Device sends 'R' on startup
		// EXPECT_TRUE(uartDeviceToHost.size() == 1);
		// EXPECT_EQ(uartDeviceToHost.front(), 'R');
		// uartDeviceToHost.pop();
		// device.OnUartRxInterrupt();
		// device.Tick();

		EXPECT_TRUE(uartDeviceToHost.empty());

		Write(uartHostToDevice, 3, serializedConfig.size(), serializedConfig);

		device.Tick();
		EXPECT_TRUE(uartDeviceToHost.empty());

		device.OnUartRxInterrupt();
		device.Tick();
		EXPECT_TRUE(uartHostToDevice.empty());
		ASSERT_TRUE(uartDeviceToHost.size() == 1);
		EXPECT_EQ(uartDeviceToHost.front(), 'A');
		uartDeviceToHost.pop();

		Write(uartHostToDevice, 0, 12, hostPayloadSerialized1);
		device.OnUartRxInterrupt();
		device.Tick();
		ASSERT_TRUE(uartDeviceToHost.size() == 0);

		device.OnRf24Interrupt();

		Write(uartHostToDevice, 12, 35, hostPayloadSerialized1);

		Write(uartHostToDevice, 0, 14, hostPayloadSerialized2);
		device.OnUartRxInterrupt();
		device.Tick();
		EXPECT_EQ(uartDeviceToHost.size(), 36);
		EXPECT_EQ(uartDeviceToHost.front(), 'A');
		uartDeviceToHost.pop();

		EXPECT_EQ(uartDeviceToHost.size(), sizeof(PayloadMessage) + 1);
		std::array<uint8_t, sizeof(PayloadMessage) + 1> receivedRf24PayloadBytes;
		for (size_t i = 0; i < receivedRf24PayloadBytes.size(); i++)
		{
			receivedRf24PayloadBytes[i] = uartDeviceToHost.front();
			uartDeviceToHost.pop();
		}
		PayloadMessage receivedRf24Payload = PayloadMessage::Deserialize(receivedRf24PayloadBytes.data(), receivedRf24PayloadBytes.size());
		EXPECT_EQ(receivedRf24Payload, devicePayloadMessage);

		Write(uartHostToDevice, 14, 35, hostPayloadSerialized2);
		device.OnUartRxInterrupt();
		device.Tick();

		EXPECT_EQ(uartDeviceToHost.size(), 1);
		EXPECT_EQ(uartDeviceToHost.front(), 'N');
		uartDeviceToHost.pop();
	}

	TEST(DeviceTest, UartBitFlipConfigurationTest)
	{
		Encoder<MaxMessageSize> encoder;
		constexpr auto configMessage = CreateConfigMessage();
		auto serializedConfig = encoder.Serialize(configMessage);
		ITimer::Handle timerHandle;
		ITimer::Elapsed timerCallback;

		std::queue<uint8_t> uartHostToDevice;
		std::queue<uint8_t> uartDeviceToHost;

		IUartMock uart;
		IRf24Mock rf24;
		ITimerMock timer;

		EXPECT_CALL(timer, SetTimer(_, _, _)).WillRepeatedly([&timerCallback, &timerHandle](auto time, auto callback, auto handle) {timerCallback = callback; timerHandle = handle; return true; });
		EXPECT_CALL(timer, ResetTimer(_)).Times(AnyNumber());

		ExpectUartReadWrite(uart, uartHostToDevice, uartDeviceToHost);
		EXPECT_CALL(uart, Flush).WillOnce([&]() {Clear(uartHostToDevice); }).WillOnce([&]() {Clear(uartHostToDevice); });
		EXPECT_CALL(rf24, PowerDown);
		EXPECT_CALL(rf24, Configure(configMessage)).WillOnce(Return(true));

		Device device(uart, rf24, timer);

		Write(uartHostToDevice, 0, 5, serializedConfig);
		device.OnUartRxInterrupt();
		device.Tick();
		Write(uartHostToDevice, 5, serializedConfig.size(), serializedConfig);
		device.OnUartRxInterrupt();
		device.OnUartRxError();
		device.Tick();

		// Uncomment if Device sends 'R' on startup
		// ASSERT_EQ(uartDeviceToHost.size(), 1);
		// EXPECT_EQ(uartDeviceToHost.front(), 'R');
		// uartDeviceToHost.pop();
		// device.Tick();

		ASSERT_EQ(uartDeviceToHost.size(), 0);

		Write(uartHostToDevice, 0, serializedConfig.size(), serializedConfig);
		device.OnUartRxInterrupt();
		device.Tick();
		ASSERT_EQ(uartDeviceToHost.size(), 0); // Device must be in Failure Lockout

		timerCallback(timerHandle);	// Will unlock the device
		device.Tick();
		ASSERT_EQ(uartDeviceToHost.size(), 1);
		EXPECT_EQ(uartDeviceToHost.front(), 'R');
		uartDeviceToHost.pop();

		Write(uartHostToDevice, 0, serializedConfig.size(), serializedConfig);
		device.OnUartRxInterrupt();
		device.Tick();

		ASSERT_EQ(uartDeviceToHost.size(), 1);
		EXPECT_EQ(uartDeviceToHost.front(), 'A');
		uartDeviceToHost.pop();
	}
}