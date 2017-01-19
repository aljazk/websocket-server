#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>

namespace net{
	
class Client{
		sf::TcpSocket socket;
	public:
		Client();
};

}
#endif