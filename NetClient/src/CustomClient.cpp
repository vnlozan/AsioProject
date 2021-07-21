#include <iostream>
#include <Net.h>

enum class CustomMessageTypes: uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage
};

class CustomClient : public Net::IClient<CustomMessageTypes> {
public:
	void PingServer() {
		Net::Message<CustomMessageTypes> msg;
		msg.header.id = CustomMessageTypes::ServerPing;
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;
		std::cout << msg << std::endl;
		Send( msg );
	}
	void MessageAll() 	{
		Net::Message<CustomMessageTypes> msg;
		msg.header.id = CustomMessageTypes::MessageAll;
		Send( msg );
	}
};




int main() {

	bool key[3] = { false, false, false };
	bool oldKey[3] = { false, false, false };

	CustomClient c;
	c.Connect( "127.0.0.1", 60000 );

	bool bQuit = false;
	while( !bQuit ) {

		if( GetForegroundWindow() == GetConsoleWindow() ) {
			key[0] = GetAsyncKeyState( '1' ) & 0x8000;
			key[1] = GetAsyncKeyState( '2' ) & 0x8000;
			key[2] = GetAsyncKeyState( '3' ) & 0x8000;
		}

		if( key[0] && !oldKey[0] ) {
			c.PingServer();
		}
		if( key[1] && !oldKey[1] ) {
			c.MessageAll();
		}
		if( key[2] && !oldKey[2] ) {
			bQuit = true;
		}
		
		for( int i = 0; i < 3; i++ ) {
			oldKey[i] = key[i];
		}

		if( c.IsConnected() ) {
			if( !c.Incoming().empty() ) {
				auto msg = c.Incoming().pop_front().msg;
				switch( msg.header.id ) {
					case CustomMessageTypes::ServerPing:{
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;
						msg >> timeThen;
						auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( timeNow - timeThen );
						std::cout << "Ping: " << milliseconds.count() << "ms" << std::endl;
					}
					break;
					case CustomMessageTypes::ServerMessage: {
						uint32_t clientID;
						msg >> clientID;
						std::cout << "Hello from [" << clientID << "]" << std::endl;
					}
					break;
					case CustomMessageTypes::ServerAccept: {			
						std::cout << "Server Accepted Connection" << std::endl;
					}
					break;
				}
			}
		} else {
			std::cout << "Server Down" << std::endl;
			bQuit = true;
		}
	}
 	return 0;
}