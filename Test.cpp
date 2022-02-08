#include "Socket.hpp"

int main() {
	InetHost host("127.0.0.1", 4555);
	UdpSocket sock;
	sock.bind(host);
	std::cout << sock.recvString() << std::endl;
	sock.outputSocketLastError(std::cout);
}