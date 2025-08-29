#include "Utils.h"
#include <gtest/gtest.h>

using namespace testing;

namespace SmartHouse::Rf24Usb::Protocol
{

	void ExpectUartReadWrite(IUartMock& uart, std::queue<uint8_t>& readQueue, std::queue<uint8_t>& writeQueue)
	{
		EXPECT_CALL(uart, Read(_))
			.WillRepeatedly([&readQueue](uint8_t& byte) {
			std::lock_guard lock(m_UartMutex);
			if (readQueue.empty())
			{
				return false;
			}
			byte = readQueue.front();
			readQueue.pop();
			return true;
				});

		EXPECT_CALL(uart, Write(_, _))
			.WillRepeatedly([&writeQueue](const uint8_t* data, size_t len) {
			std::lock_guard lock(m_UartMutex);
			for (size_t i = 0; i < len; i++)
			{
				writeQueue.push(data[i]);
			}
				});
	}

	void Clear(std::queue<uint8_t>& queue)
	{
		std::lock_guard lock(m_UartMutex);
		while (!queue.empty())
		{
			queue.pop();
		}
	}
}