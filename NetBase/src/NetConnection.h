#pragma once
#include "NetTSQueue.h"
#include "NetMessage.h"
#include "NetMessageOwner.h"

namespace Net {

	template<typename T>
	class IServer;

	template<typename T>
	class Connection : public std::enable_shared_from_this<Connection<T>> {
	public:
		enum class Owner {
			Server,
			Client
		};

		Connection(
			Owner parent,
			asio::io_context& asioContext,
			asio::ip::tcp::socket socket,
			Net::TSQueue<Net::MessageOwner<T>>& qIn
		): 
			m_AsioContext{ asioContext },
			m_Socket{ std::move( socket ) },
			m_qMessagesIn{ qIn },
			m_OwnerType{ std::move( parent ) }
		{
			if( m_OwnerType == Owner::Server ) {
				m_nHandshakeOut = uint64_t( std::chrono::system_clock::now().time_since_epoch().count() );
				m_nHandshakeCheck = scramble( m_nHandshakeOut );
			} else {
				m_nHandshakeIn = 0;
				m_nHandshakeOut = 0;
			}
		}

		virtual ~Connection() {}
		void ConnectToClient( Net::IServer<T>* server, uint32_t uid = 0 ) {
			if( m_OwnerType == Owner::Server ) {
				if( m_Socket.is_open() ) {
					id = uid;
					//ReadHeader();
					WriteValidation();
					ReadValidation( server );
				}
			}
		}
		void ConnectToServer( const asio::ip::tcp::resolver::results_type& endpoints ) {
			// Only clients connect to servers
			if( m_OwnerType == Owner::Client ) {
				// Request asio attempts to connect to an endpoint
				asio::async_connect( m_Socket, endpoints,
					[this] ( std::error_code ec, asio::ip::tcp::endpoint endpoint ) {
						if( !ec ) {
							//ReadHeader();
							ReadValidation();
						}
					}
				);
			}
		}
		void Disconnect() {
			if( IsConnected() ) {
				asio::post( m_AsioContext, [this] () { m_Socket.close(); } );
			}
		}
		bool IsConnected() const {
			return m_Socket.is_open();
		}
		void Send( const Net::Message<T>& msg ) {
			asio::post( m_AsioContext,
				[this, msg] () {
					bool bWritingMessage = !m_qMessagesOut.empty();
					m_qMessagesOut.push_back( msg );
					if( !bWritingMessage ) {
						WriteHeader();
					}
				}
			);
		}
		inline uint32_t GetID() const { return id; }
	private:
		// ASYNC - Prime context ready to read a message header
		void ReadHeader() {
			asio::async_read( m_Socket, asio::buffer( &m_MsgTmpIn.header, sizeof( Net::MessageHeader<T> ) ),
				[this] ( std::error_code ec, std::size_t length ) {
				if( !ec ) {
					if( m_MsgTmpIn.header.size > 0 ) {
						m_MsgTmpIn.body.resize( m_MsgTmpIn.header.size );
						ReadBody();
					} else {
						AddToIncomingMessageQueue();
					}
				} else {
					std::cout << "[" << id << "] Read Header Fail." << std::endl;
					m_Socket.close();
				}
			}
			);
		}
		// ASYNC - Prime context ready to read a message body
		void ReadBody() {
			//std::cout << "READ BODY SIZE: " << m_MsgTmpIn.header.size << std::endl;
			asio::async_read( m_Socket, asio::buffer( m_MsgTmpIn.body.data(), m_MsgTmpIn.header.size ),
				[this] ( std::error_code ec, std::size_t length ) {
				if( !ec ) {
					AddToIncomingMessageQueue();
				} else {
					std::cout << "[" << id << "] Read Body Fail." << std::endl;
					m_Socket.close();
				}
			}
			);
		}
		// ASYNC - Prime context ready to write a message header
		void WriteHeader() {
			asio::async_write( m_Socket, asio::buffer( &m_qMessagesOut.front().header, sizeof( Net::MessageHeader<T> ) ),
				[this] ( std::error_code ec, std::size_t length ) {
				if( !ec ) {
					if( m_qMessagesOut.front().body.size() > 0 ) {
						WriteBody();
					} else {
						m_qMessagesOut.pop_front();
						if( !m_qMessagesOut.empty() ) {
							WriteHeader();
						}
					}
				} else {
					std::cout << "[" << id << "] Write Header Fail." << std::endl;
					m_Socket.close();
				}
			}
			);
		}
		// ASYNC
		void ReadValidation( Net::IServer<T>* server = nullptr ) {
			asio::async_read( m_Socket, asio::buffer( &m_nHandshakeIn, sizeof( uint64_t ) ),
				[this, server] (std::error_code ec, std::size_t length) {
					if( !ec ) {
						if( m_OwnerType == Owner::Server ) {
							if( m_nHandshakeIn == m_nHandshakeCheck ) {

								std::cout << "[" << id << "] Validated." << std::endl;
								
								server->OnClientValidated( this->shared_from_this() );
								// Wait for data now
								ReadHeader();
							} else {
								std::cout << "[" << id << "] Failed Validation." << std::endl;
								m_Socket.close();
							}
						} else {
							m_nHandshakeOut = scramble( m_nHandshakeIn );
							WriteValidation();
						}
					} else {
						std::cout << "[" << id << "] Disconnected (ReadValidation)." << std::endl;
						m_Socket.close();
					}
				}
			);
		}

		// ASYNC - Prime context ready to write a message body
		void WriteBody() {
			//std::cout << "WRITE BODY SIZE: " << m_qMessagesOut.front().header.size << std::endl;
			asio::async_write( m_Socket, asio::buffer( m_qMessagesOut.front().body.data(), m_qMessagesOut.front().header.size ),
				[this] ( std::error_code ec, std::size_t length ) {
				if( !ec ) {
					m_qMessagesOut.pop_front();
					if( !m_qMessagesOut.empty() ) {
						WriteHeader();
					}
				} else {
					std::cout << "[" << id << "] Write Body Fail." << std::endl;
					m_Socket.close();
				}
			}
			);
		}
		// ASYNC - Used by both client and server to write validation packet
		void WriteValidation() {
			asio::async_write( m_Socket, asio::buffer( &m_nHandshakeOut, sizeof( uint64_t ) ), 
				[this]( std::error_code ec, std::size_t length ) {
					if( !ec ) {
						if( m_OwnerType == Owner::Client ) {
							ReadHeader();
						}
					} else {
						m_Socket.close();
					}
				}
			);
		}

		void AddToIncomingMessageQueue() {
			if( m_OwnerType == Owner::Server ) {
				m_qMessagesIn.push_back( { this->shared_from_this(), m_MsgTmpIn } );
			} else {
				m_qMessagesIn.push_back( { nullptr, m_MsgTmpIn } );
			}
			ReadHeader();
		}

		// Encrypt Data
		uint64_t scramble( uint64_t nInput ) {
			uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
			out = ( out & 0xF0F0F0F0F0F0F0 ) >> 4 | ( out & 0xF0F0F0F0F0F0F ) << 4;
			return out ^ 0xC0DEFACE12345678;
		}
	protected:
		// Each connection has a unique socket to a remote
		asio::ip::tcp::socket m_Socket;
		// This context is shared with the whole asio instance
		asio::io_context& m_AsioContext;
		
		// This queue holds all messages to be sent
		// to the remote side of this connection
		Net::TSQueue<Net::Message<T>> m_qMessagesOut;
		
		// This queue holds all messages that have been received
		// the remote side of this connection. Note it is a reference
		// as the "owner" of this connection is expected to provide a queue
		Net::TSQueue<Net::MessageOwner<T>>& m_qMessagesIn;
		
		Net::Message<T> m_MsgTmpIn;

		// The "owner" decides how some of the connections behaves
		Owner m_OwnerType = Owner::Server;

		uint32_t id = 0;

		// Handshake
		uint64_t m_nHandshakeOut = 0;
		uint64_t m_nHandshakeIn = 0;
		uint64_t m_nHandshakeCheck = 0;
	};
}