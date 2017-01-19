#ifndef NETWORK_H
#define NETWORK_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <memory>
#include <SFML/System.hpp>

#include "Client.hpp"

namespace net{

class Network{
		unsigned int port;
		sf::SocketSelector selector;
		std::unordered_set<std::unique_ptr<sf::TcpSocket>> sockets;
		bool handshake(sf::TcpSocket &);
		std::string unmask(const unsigned char *, const unsigned char *, unsigned long int);
		void mask(unsigned char *, const unsigned char *, long unsigned int);
		void send(sf::TcpSocket &, std::string);
		void listen();
		sf::Mutex mutex;
	public:
		Network();
		void run();
};

}

#endif