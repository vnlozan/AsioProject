#include <iostream>
#include <Net.h>


enum class CustomMessageTypes: uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage
};

class CustomServer: public Net::IServer<CustomMessageTypes> {
public:
	CustomServer( uint16_t nPort ): Net::IServer<CustomMessageTypes>{ nPort } {

	}
protected:
	bool OnClientConnect( std::shared_ptr<Net::Connection<CustomMessageTypes>> client ) override {
		Net::Message<CustomMessageTypes> msg;
		msg.header.id = CustomMessageTypes::ServerAccept;
		client->Send( msg );
		return true;
	}
	void OnClientDisconnect( std::shared_ptr<Net::Connection<CustomMessageTypes>> client ) override {
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}
	virtual void OnMessage( std::shared_ptr<Net::Connection<CustomMessageTypes>> client, Net::Message<CustomMessageTypes>& msg ) override {
		switch( msg.header.id ) {
			case CustomMessageTypes::ServerPing: {
				std::cout << "[" << client->GetID() << "] : Server Ping" << std::endl;
				client->Send( msg );
			}
			break;
			case CustomMessageTypes::MessageAll: {
				std::cout << "[" << client->GetID() << "] : Message All\n";
				Net::Message<CustomMessageTypes> msg;
				msg.header.id = CustomMessageTypes::ServerMessage;
				msg << client->GetID();
				MessageAllClients( msg, client );
			}
			break;
		}
	}
};

int main() {
	CustomServer server{ 60000 };
	server.Start();
	while( 1 ) {
		server.Update( -1, true );
	}
	return 0;
}