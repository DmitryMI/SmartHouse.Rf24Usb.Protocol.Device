#pragma once

#include <cstdint>
#include <queue>
#include <chrono>
#include <functional>
#include <atomic>

#include "SmartHouse/Rf24Usb/Protocol/ConfigMessage.h"
#include "SmartHouse/Rf24Usb/Protocol/IRf24.h"
#include "SmartHouse/Rf24Usb/Protocol/IUart.h"
#include "SmartHouse/Rf24Usb/Protocol/ITimer.h"
#include "SmartHouse/Rf24Usb/Protocol/Encoder.h"
#include "SmartHouse/Rf24Usb/Protocol/Decoder.h"
#include "SmartHouse/Rf24Usb/Protocol/Constants.h"

// UART Assumptions:
// 1. Bit flips are possible.
// 2. Bit and byte omissions are not possible.
// 3. All messages must be transmitted within 500ms.
// 4. UART is full-duplex and can write while reading in parallel.

namespace SmartHouse::Rf24Usb::Protocol
{

	class Device
	{
	public:
		Device(IUart& uart, IRf24& rf24, ITimer& timer);

		void OnUartRxInterrupt();

		void OnUartRxError();

		void OnRf24Interrupt();

		void Tick();

	private:
		IUart& m_Uart;
		IRf24& m_Rf24;
		ITimer& m_Timer;

		std::atomic<bool> m_UartIrq = false;
		std::atomic<bool> m_UartError = false;
		std::atomic<bool> m_Rf24Irq = false;
		std::atomic<bool> m_TimerElapsed = false;
		Encoder<0> m_UartEncoder;
		Decoder<MaxMessageSize> m_UartDecoder;
		bool m_IsConfigured = false;
		bool m_FailureLockout = false;
		ITimer::Elapsed m_TimerElapsedCallback = [&](ITimer::Handle handle) {OnTimerElapsed(handle); };
		ITimer::Handle m_TimerHandle{ITimer::InvalidHandle};

		void OnTimerElapsed(ITimer::Handle handle);

		void UpdateTimer();

		void OnError();

		void OnByteReceived(uint8_t byte);

		void OnConfigReceived(const ConfigMessage& config);

		void OnPayloadReceived(const PayloadMessage& payload);

		void WriteAck(bool ok);

		void WriteReset();

		void WritePayload(uint8_t pipe, uint8_t size, const std::array<uint8_t, 32>& data);
	};
}