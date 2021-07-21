#pragma once
#include "NetMessage.h"
#include "NetMessageOwner.h"
#include "NetTSQueue.h"
#include "NetConnection.h"

namespace Net {
	template<typename T>
	class IClient {
	public:
		IClient() : m_Socket{ m_Context } {
			// Initialize the socket with the io context, so it can do stuff
		}
		virtual ~IClient() {
			// If the client is destroyed, always try and disconnect from server
			Disconnect();
		}
		bool Connect(const std::string& host, const uint16_t port) {
			try {
				// Resolve hostname/ip-address into tangiable physical addreess
				asio::ip::tcp::resolver resolver( m_Context );
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve( host, std::to_string( port ) );
				// Create connection
				m_Connection = std::make_unique<Net::Connection<T>>(
					Net::Connection<T>::Owner::Client,
					m_Context,
					asio::ip::tcp::socket( m_Context ),
					m_qMessagesIn
				);
				// Tell the connection object to connect to server
				m_Connection->ConnectToServer( endpoints );
				// Start Context Thread
				threadContext = std::thread([this]() { m_Context.run(); });
			} catch (std::exception& e) {
				std::cerr << "Client Exception: " << e.what() << "\r\n";
				return false;
			}
			return false;
		}
		void Disconnect() {
			if (IsConnected()) {
				m_Connection->Disconnect();
			}
			m_Context.stop();
			if (threadContext.joinable()) {
				threadContext.join();
			}
			m_Connection.release();
		}
		bool IsConnected() {
			if (m_Connection) {
				return m_Connection->IsConnected();
			}
			return false;
		}
		Net::TSQueue<Net::MessageOwner<T>>& Incoming() {
			return m_qMessagesIn;
		}
		void Send(const Net::Message<T>& msg) {
			if( IsConnected() ) {
				//std::cout << "Sending request" << std::endl;
				m_Connection->Send(msg);
			}
		}
	protected:
		// asio context handles the data transfer
		asio::io_context m_Context;
		// but needs a thread of its own to execute its work commands
		std::thread threadContext;
		// This is the hardware socket that is connected to the server
		asio::ip::tcp::socket m_Socket;
		// The client has a single instance of a connection object, which handles data transfer
		std::unique_ptr<Net::Connection<T>> m_Connection;
	private:
		// This is the thread safe queue of incoming messages from server
		Net::TSQueue<Net::MessageOwner<T>> m_qMessagesIn;
	};
}