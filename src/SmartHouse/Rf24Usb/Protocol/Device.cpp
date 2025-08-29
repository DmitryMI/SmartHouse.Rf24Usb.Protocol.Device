#include "SmartHouse/Rf24Usb/Protocol/Device.h"

namespace SmartHouse::Rf24Usb::Protocol
{
	Device::Device(
		IUart& uart,
		IRf24& rf24,
		ITimer& timer
	) :
		m_Uart(uart),
		m_Rf24(rf24),
		m_Timer(timer)
	{
		// WriteReset();
	}

	void Device::OnUartRxInterrupt()
	{
		m_UartIrq = true;
	}

	void Device::OnUartRxError()
	{
		m_UartError = true;
	}

	void Device::OnRf24Interrupt()
	{
		m_Rf24Irq = true;
	}

	void Device::Tick()
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
				std::vector<Rf24Packet> receivedPackets;
				uint8_t pipe;
				uint8_t size;
				std::array<uint8_t, 32> data;
				while (m_Rf24.Read(pipe, size, data))
				{
					receivedPackets.push_back({ pipe, size, data });
				}

				for (const auto& packet : receivedPackets)
				{
					WritePayload(packet.m_Pipe, packet.m_Size, packet.m_Data);
				}
			}
		}
	}


	void Device::OnTimerElapsed(ITimer::Handle handle)
	{
		(void)handle;
		m_TimerElapsed = true;
	}

	void Device::UpdateTimer()
	{
		bool setOk = m_Timer.SetTimer(std::chrono::milliseconds(500), m_TimerElapsedCallback, m_TimerHandle);
		assert(setOk);
	}

	void Device::OnError()
	{
		m_Uart.Flush();
		m_Rf24.PowerDown();
		m_Timer.ResetTimer(m_TimerHandle);
		m_IsConfigured = false;
		m_UartDecoder.Reset();

		m_FailureLockout = true;
		m_Timer.SetTimer(DeviceFailureLockoutDuration, m_TimerElapsedCallback, m_TimerHandle);
	}

	void Device::OnByteReceived(uint8_t byte)
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

	void Device::OnConfigReceived(const ConfigMessage& config)
	{
		bool ok = m_Rf24.Configure(config);
		m_IsConfigured = ok;
		WriteAck(ok);
	}

	void Device::OnPayloadReceived(const PayloadMessage& payload)
	{
		if (!m_IsConfigured)
		{
			OnError();
			return;
		}
		bool ack = m_Rf24.Write(payload.m_Size, payload.m_Data);
		WriteAck(ack);
	}

	void Device::WriteAck(bool ok)
	{
		std::array<uint8_t, 1> ackBytes;
		if (ok)
		{
			AckMessage ackMessage;
			ackBytes = m_UartEncoder.Serialize(ackMessage);
		}
		else
		{
			NackMessage nackMessage;
			ackBytes = m_UartEncoder.Serialize(nackMessage);
		}

		m_Uart.Write(ackBytes.data(), ackBytes.size());
	}

	void Device::WriteReset()
	{
		ResetMessage resetMessage;
		auto resetBytes = m_UartEncoder.Serialize(resetMessage);
		m_Uart.Write(resetBytes.data(), resetBytes.size());
	}

	void Device::WritePayload(uint8_t pipe, uint8_t size, const std::array<uint8_t, 32>& data)
	{
		PayloadMessage message{ pipe, size, data };
		auto bytes = m_UartEncoder.Serialize(message);
		m_Uart.Write(bytes.data(), bytes.size());
	}
}