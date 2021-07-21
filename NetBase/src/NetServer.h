#pragma once
#include "NetBase.h"
#include "NetTSQueue.h"
#include "NetMessage.h"
#include "NetConnection.h"
#include "NetMessageOwner.h"

namespace Net {
	template<typename T>
	class IServer {
	public:
		IServer( uint16_t port )
			: m_AsioAcceptor{ m_AsioContext, asio::ip::tcp::endpoint{ asio::ip::tcp::v4(), port } } {
		}
		virtual ~IServer(){
			Stop();
		}
		bool Start() {
			try {
				WaitForClientConnection();
				m_ThreadContext = std::thread( [this] () { m_AsioContext.run(); } );
			} catch( std::exception& e ) {
				std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
				return false;
			}
			std::cout << "[SERVER] Started!" << std::endl;
			return true;
		}
		void Stop() {
			m_AsioContext.stop();
			if( m_ThreadContext.joinable() ) {
				m_ThreadContext.join();
			}
			std::cout << "[SERVER] Stopped!" << std::endl;
		}
		void WaitForClientConnection() {
			m_AsioAcceptor.async_accept( [this] ( std::error_code ec, asio::ip::tcp::socket socket ) {
				if( !ec ) {
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
					std::shared_ptr<Net::Connection<T>> newConnection = std::make_shared<Net::Connection<T>>(
						Net::Connection<T>::Owner::Server,
						m_AsioContext,
						std::move( socket ),
						m_qMessagesIn
					);
					if( OnClientConnect( newConnection ) ) {
						m_DeqConnections.push_back( std::move( newConnection ) );
						m_DeqConnections.back()->ConnectToClient( this, clientIdCounter++ );
						std::cout << "[" << m_DeqConnections.back()->GetID() << "] Connection Approved!" << std::endl;
					} else {
						std::cout << "[---] Connection denied" << std::endl;
					}
				} else {
					std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;
				}
				// Wait for another connection
				WaitForClientConnection();
			} );
		}
		void MessageClient( std::shared_ptr<Net::Connection<T>> client, const Net::Message<T>& msg ) {
			if( client && client->IsConnected() ) {
				client->Send( msg );
			} else {
				OnClientDisconnect( client );
				client.reset();
				m_DeqConnections.erase( std::remove(m_DeqConnections.begin(), m_DeqConnections.end(), client ), m_DeqConnections.end() );
			}
		}
		void MessageAllClients( const Net::Message<T>& msg, std::shared_ptr<Net::Connection<T>> pIgnoredClient = nullptr ) {
			bool bInvalidClientExists = false;
			for( auto& client : m_DeqConnections ) {
				if( client && client->IsConnected() ) {
					if( client != pIgnoredClient ) {
						client->Send( msg );
					}
				} else {
					OnClientDisconnect( client );
					client.reset();
					bInvalidClientExists = true;
				}
			}
			if( bInvalidClientExists ) {
				m_DeqConnections.erase( std::remove( m_DeqConnections.begin(), m_DeqConnections.end(), nullptr ), m_DeqConnections.end() );
			}
		}
		void Update( size_t nMaxMessages = -1, bool bWait = false ) {
			if( bWait ) {
				m_qMessagesIn.wait();
			}
			size_t nMessagesCount = 0;
			while( nMessagesCount < nMaxMessages && !m_qMessagesIn.empty() ) {
				// Grab the front message
				auto msg = m_qMessagesIn.pop_front();

				// Pass to message handler
				OnMessage( msg.remote, msg.msg );

				nMessagesCount++;
			}
		}
	public:
		// Called when client validated
		virtual void OnClientValidated( std::shared_ptr<Net::Connection<T>> client ) {

		}
	protected:
		// Called when a client connects
		virtual bool OnClientConnect( std::shared_ptr<Net::Connection<T>> client ) {
			return false;
		}
		// Called when a client appears to have disconnected
		virtual void OnClientDisconnect( std::shared_ptr<Net::Connection<T>> client ) {

		}
		// Called when a message arrives
		virtual void OnMessage( std::shared_ptr<Net::Connection<T>> client, Net::Message<T>& msg ) {

		}
	protected:
		uint32_t clientIdCounter = 10000;
		Net::TSQueue<Net::MessageOwner<T>> m_qMessagesIn;
		std::deque<std::shared_ptr<Net::Connection<T>>> m_DeqConnections;

		// Order of declaration( initialization ) is important
		asio::io_context m_AsioContext;
		std::thread m_ThreadContext;

		asio::ip::tcp::acceptor m_AsioAcceptor;
	};
}