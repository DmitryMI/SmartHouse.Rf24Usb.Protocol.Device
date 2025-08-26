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
		Device(
			IUart& uart, 
			IRf24& rf24,
			ITimer& timer
			):
			m_Uart(uart),
			m_Rf24(rf24),
			m_Timer(timer)
		{
			// WriteReset();
		}

		void OnUartRxInterrupt()
		{
			m_UartIrq = true;
		}

		void OnUartRxError()
		{
			m_UartError = true;
		}

		void OnRf24Interrupt()
		{
			m_Rf24Irq = true;
		}

		void Tick()
		{
			bool flagExpected = true;

			if (m_FailureLockout)
			{
				if (m_TimerElapsed.compare_exchange_weak(flagExpected, false))
				{
					m_Uart.Flush();
					m_Timer.ResetTimer(m_TimerHandle);
					m_FailureLockout = false;
					WriteReset();
				}
				else
				{
					return;
				}
			}			
			
			if (m_UartIrq.compare_exchange_weak(flagExpected, false))
			{
				uint8_t b;
				while (m_Uart.Read(b) && !m_UartError)
				{
					UpdateTimer();
					OnByteReceived(b);
				}
			}

			m_Timer.ResetTimer(m_TimerHandle);

			flagExpected = true;
			if (m_UartError.compare_exchange_weak(flagExpected, false))
			{
				OnError();
				return;
			}

			flagExpected = true;
			if (m_TimerElapsed.compare_exchange_weak(flagExpected, false))
			{
				m_UartDecoder.Reset();
			}

			flagExpected = true;
			if (m_Rf24Irq.compare_exchange_weak(flagExpected, false))
			{
				bool txOk, txFail, rxOk;
				m_Rf24.WhatHappened(txOk, txFail, rxOk);
				if (rxOk)
				{
					uint8_t pipe;
					uint8_t size;
					std::array<uint8_t, 32> data;
					while (m_Rf24.Read(pipe, size, data))
					{
						WritePayload(pipe, size, data);
					}
				}
			}
		}

	private:
		IUart& m_Uart;
		IRf24& m_Rf24;
		ITimer& m_Timer;

		std::atomic<bool> m_UartIrq = false;
		std::atomic<bool> m_UartError = false;
		std::atomic<bool> m_Rf24Irq = false;
		std::atomic<bool> m_TimerElapsed = false;
		Decoder m_UartDecoder;
		bool m_IsConfigured = false;
		bool m_FailureLockout = false;
		ITimer::Elapsed m_TimerElapsedCallback = [&](ITimer::Handle handle) {OnTimerElapsed(handle); };
		ITimer::Handle m_TimerHandle{ITimer::InvalidHandle};

		void OnTimerElapsed(ITimer::Handle handle)
		{
			(void)handle;
			m_TimerElapsed = true;
		}

		void UpdateTimer()
		{
			bool setOk = m_Timer.SetTimer(std::chrono::milliseconds(500), m_TimerElapsedCallback, m_TimerHandle);
			assert(setOk);
		}

		void OnError()
		{
			m_Uart.Flush();
			m_Rf24.PowerDown();
			m_Timer.ResetTimer(m_TimerHandle);
			m_IsConfigured = false;
			m_UartDecoder.Reset();

			m_FailureLockout = true;
			m_Timer.SetTimer(DeviceFailureLockoutDuration, m_TimerElapsedCallback, m_TimerHandle);
		}

		void OnByteReceived(uint8_t byte)
		{
			m_UartDecoder.PushByte(byte);
			if (!m_UartDecoder.IsCurrentMessageHeaderValid())
			{
				OnError();
				return;
			}

			if (!m_UartDecoder.IsMessageComplete())
			{
				return;
			}
			auto header = m_UartDecoder.GetCurrentMessageHeader().value();
			switch (header)
			{
			case MessageHeader::Config:
			{
				auto config = m_UartDecoder.TryDecodeConfig().value();
				OnConfigReceived(config);
				m_UartDecoder.Reset();
				break;
			}
			case MessageHeader::Payload:
			{
				auto payload = m_UartDecoder.TryDecodePayload().value();
				OnPayloadReceived(payload);
				m_UartDecoder.Reset();
				break;
			}
			default:
				OnError();
				break;
			}
		}

		void OnConfigReceived(const ConfigMessage& config)
		{
			bool ok = m_Rf24.Configure(config);
			m_IsConfigured = ok;
			WriteAck(ok);
		}

		void OnPayloadReceived(const PayloadMessage& payload)
		{
			if (!m_IsConfigured)
			{
				OnError();
				return;
			}
			bool ack = m_Rf24.Write(payload.m_Size, payload.m_Data);
			WriteAck(ack);
		}

		void WriteAck(bool ok)
		{
			std::array<uint8_t, 1> ackBytes;
			if (ok)
			{
				AckMessage ackMessage;
				ackMessage.Serialize(ackBytes.data(), ackBytes.size());
			}
			else
			{
				NackMessage nackMessage;
				nackMessage.Serialize(ackBytes.data(), ackBytes.size());
			}

			m_Uart.Write(ackBytes.data(), ackBytes.size());
		}

		void WriteReset()
		{
			std::array<uint8_t, 1> resetBytes;
			ResetMessage resetMessage;
			resetMessage.Serialize(resetBytes.data(), resetBytes.size());
			m_Uart.Write(resetBytes.data(), resetBytes.size());
		}

		void WritePayload(uint8_t pipe, uint8_t size, const std::array<uint8_t, 32>& data)
		{
			PayloadMessage message{ pipe, size, data };
			std::array<uint8_t, sizeof(PayloadMessage) + 1> bytes;
			message.Serialize(bytes.data(), bytes.size());
			m_Uart.Write(bytes.data(), bytes.size());
		}
	};
}