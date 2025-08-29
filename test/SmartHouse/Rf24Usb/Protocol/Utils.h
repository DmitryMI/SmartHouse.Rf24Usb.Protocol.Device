#pragma once

#include "IRf24Mock.h"
#include "IUartMock.h"
#include "ITimerMock.h"
#include <queue>
#include "SmartHouse/Rf24Usb/Protocol/ConfigMessage.h"
#include "SmartHouse/Rf24Usb/Protocol/PayloadMessage.h"
#include <mutex>

namespace SmartHouse::Rf24Usb::Protocol
{
	inline std::mutex m_UartMutex;

	constexpr ConfigMessage CreateConfigMessage()
	{
		ConfigMessage config;
		config.m_Flags = ConfigMessage::Flags::RxPipe3 | ConfigMessage::Flags::RxPipe5;
		config.m_PipeTxAddress = { 'P', 'H', 'T', 'S', '0' };
		config.m_PipeRxAddressPrimary = { 'P', 'H', 'T', 'C', '0' };
		config.m_PipeRxAddressLsb = { 'A', 'B', 'C', 'D', 'E' };
		config.m_Channel = 0xAB;
		config.m_PaLevel = ConfigMessage::PaLevel::Low;
		config.m_DataRate = ConfigMessage::DataRate::Rate2mbps;
		config.m_RetryDelay = 8;
		config.m_RetryCount = 12;
		return config;
	}

	constexpr PayloadMessage CreateHostPayloadMessage(size_t size, size_t offset)
	{
		PayloadMessage payload;
		payload.m_Pipe = 0;
		payload.m_Size = size;
		for (size_t i = 0; i < size; i++)
		{
			payload.m_Data[i] = i + offset;
		}
		return payload;
	}

	template<size_t Size>
	void Write(std::queue<uint8_t>& queue, size_t from, size_t to, const std::array<uint8_t, Size>& arr)
	{
		for (size_t i = from; i < to; i++)
		{
			queue.push(arr[i]);
		}
	}

	void ExpectUartReadWrite(IUartMock& uart, std::queue<uint8_t>& readQueue, std::queue<uint8_t>& writeQueue);
	void Clear(std::queue<uint8_t>& queue);
}