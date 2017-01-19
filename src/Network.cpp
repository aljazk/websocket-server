#include "Network.hpp"

using namespace net;

Network::Network(){
	port = 80;
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

std::string accept(std::string key){
	//std::cout << "WebSocket-Accept:" << std::endl;
	std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	std::string start = key + magic;
	std::string input = "echo|set /p=\""+start+"\" | openssl dgst -sha1 | sed 's/^.* //'";
	std::string sha1 = exec(input.c_str());
	sha1.pop_back();
	input = "echo "+sha1+" | xxd -r -p | openssl base64 | tr -d \'\\n\'";
	std::string base64 = exec(input.c_str());
	/*
	std::cout << "Start: " << start << std::endl;
	std::cout << "SHA-1: " << sha1 << std::endl;
	std::cout << "base64: " << base64 << std::endl;
	*/
	return base64;
}

bool Network::handshake(sf::TcpSocket &socket){
	char data[256];
	std::size_t received;
	std::string rdata;
	std::string key;
	while(socket.receive(data, 256, received) == sf::Socket::Done){
		//std::cout << "Data size: " << received << std::endl;
		rdata = data;
		std::size_t found = rdata.find("\r\n\r\n");
		if(found != std::string::npos){
			rdata.resize(found);
		}
		//std::cout << "Data: " << rdata << std::endl;
		std::size_t rfound = rdata.find("Sec-WebSocket-Key");
		if (rfound != std::string::npos){
			bool push = false;
			for(unsigned int i=rfound; i<rdata.length(); i++){
				if (rdata[i] == '\n') break;
				if (rdata[i] == ' ') push = true;
				if (push) key.push_back(rdata[i]);
			}
		}
		if (found != std::string::npos){
			break;
		}
	}
	if(key.length() == 0){
		std::cout << "Didnt recive key from client" << std::endl;
		return false;
	}
	//std::cout << "Recived key from client: " << key << std::endl;
	key = accept(key);
	std::ostringstream sdata;
	sdata << "HTTP/1.1 101 Switching Protocols\r\n"
		<< "Upgrade: websocket\r\n"
		<< "Connection: Upgrade\r\n"
		<< "Sec-WebSocket-Accept: " << key << "\r\n\r\n";
	std::string test = sdata.str();
	//std::cout << "TEST:" << test << "\nLENGTH: " << test.length() << std::endl;
	if (socket.send(test.c_str(), test.length()) != sf::Socket::Done){
		std::cout << "Cannot send data | Error " << sf::Socket::Error << std::endl;
		return false;
	} else {
		std::cout << "Handshake was complete" << std::endl;
	}
	return true;
}

std::string Network::unmask(const unsigned char *data, const unsigned char *mask, unsigned long int length){
	std::string return_data;
	for(unsigned long int i=0; i<length; i++){
		return_data.push_back(data[i] ^ mask[i%4]);
	}
	return return_data;
}

void Network::mask(unsigned char *data, const unsigned char *mask, long unsigned int length){
	for(unsigned int i=0; i<length; i++){
		data[i] = data[i] ^ mask[i%4];
	}
}

void Network::send(sf::TcpSocket &socket, std::string data){
	unsigned char abnf[2];
	unsigned char ldata[4];
	abnf[0] = 129;
	std::string test;
	test.push_back(abnf[0]);
	unsigned int length = data.length();
	if(length < 126){
		abnf[1] = length;
		test.push_back(abnf[1]);
	} else { 
		ldata[0] = (length >> 24) & 0xFF;
		ldata[1] = (length >> 16) & 0xFF;
		ldata[2] = (length >> 8) & 0xFF;
		ldata[3] = length & 0xFF;
		if (length > 126 && length < 65535){
			abnf[1] = 126;
			test.push_back(abnf[1]);
			test.push_back(ldata[2]);
			test.push_back(ldata[3]);
		} else {
			abnf[1] = 127;
			test.push_back(abnf[1]);
			test.push_back(ldata[0]);
			test.push_back(ldata[1]);
			test.push_back(ldata[2]);
			test.push_back(ldata[3]);
		}
	}
	test += data;
	std::size_t size_send;
	if(socket.send(test.c_str(), test.length(), size_send) == sf::Socket::Done){
		std::cout << "Send to client: " << test << std::endl;
		if(test.length() != size_send){
			std::cout << "Not all data was send (data length: " << test.length() << ", data send: " << size_send << ")" << std::endl;
		}
	}
}

void Network::listen(){
	sf::TcpListener listener;
	//start listening to port
	if (listener.listen(port) != sf::Socket::Done){
		std::cout << "Cannot listen to port" << std::endl;
	} else {
		while(true){
			std::unique_ptr<sf::TcpSocket> socket(new sf::TcpSocket);
			mutex.lock();
			std::cout << "\nListening for new client.." << std::endl;
			mutex.unlock();
			if (listener.accept(*socket) != sf::Socket::Done){
				std::cout << "\nCannot connect to new client" << std::endl;
			} else {
				std::cout << "\nConnected to new client" << std::endl;
				if (handshake(*socket)){
					mutex.lock();
					socket->setBlocking(false);
					sockets.emplace(std::move(socket));
					//selector.add(*socket);
					std::cout << "Client was added " << sockets.size()  << std::endl;
					mutex.unlock();
					//end handshake
					//break;
				}
			}
		}
	}
}

void Network::run(){
	sf::Thread listen_thread(&listen, this);
	listen_thread.launch();
	std::cout << "Runing.." << std::endl;
	std::size_t received;
	while(true){
		mutex.lock();
		for (const auto& s: sockets){
			unsigned char head[2];
			if(s->receive(head, 2, received) == sf::Socket::Done){
				std::cout << "\nData was received from cilent:\nABNF: ";
				for(unsigned int i=0; i<received; i++){
					std::cout << (int)head[i] << " ";
				}
				std::cout << std::endl;
				if(head[0] == 136) {
					std::cout << "Connection broke up" << std::endl;
					s->disconnect();
					//sockets.erase(s);
				}
				//get length
				unsigned char cdata = head[1];
				cdata = cdata << 1;
				cdata = cdata >> 1;
				unsigned long int length = 0;
				if (cdata < 126){
					length = cdata;
				} else if (cdata == 126){
					unsigned short int g;
					if(s->receive(&g, 1, received) == sf::Socket::Done){
						length = g;
					}
				} else if (cdata == 127){
					if(s->receive(&length, 1, received) == sf::Socket::Done){
						
					}
				}
				std::cout << "length of a message: " << length << std::endl;
				unsigned char cmask[4];
				if(s->receive(cmask, 4, received) == sf::Socket::Done){
					std::cout << "Mask: ";
					for(unsigned int i=0; i<received; i++){
						std::cout << (int)cmask[i] << " ";
					}
				}
				unsigned char data[length];
				if(s->receive(data, length, received) == sf::Socket::Done){
					std::cout << "\nData: ";
					for(unsigned long int i=0; i<received; i++){
						std::cout << (int)data[i] << " ";
					}
					std::cout << "\nUnmasked data: " << unmask(data, cmask, length);
				}
				std::cout << std::endl;
				//say hello
				send(*s, "hello");
			}
		}
		//check erase
		for(auto s = sockets.begin(); s != sockets.end(); ){
			if((*s)->getRemotePort() == 0){
				s = sockets.erase(s);
				std::cout << "Socket was removed " << sockets.size() << std::endl;
			} else {
				++s;
			}
		}
		mutex.unlock();
	}
}