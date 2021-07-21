#pragma once
#include "NetMessage.h"

namespace Net {
	template<typename T>
	class Connection;

	template<typename T>
	struct MessageOwner {
		std::shared_ptr<Net::Connection<T>> remote = nullptr;
		Net::Message<T> msg;

		friend std::ostream& operator<<(std::ostream& os, const MessageOwner<T>& msg) {
			os << msg.msg;
			return os;
		}
	};
}