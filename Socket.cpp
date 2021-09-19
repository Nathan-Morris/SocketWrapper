#include "Socket.hpp"

InetAddress::InetAddress(const char* host) { 
    inet_pton(AF_INET, host, (void*)&this->_address);
}

InetAddress::InetAddress(const std::string& host) 
:   InetAddress(host.c_str()) { }

InetAddress::InetAddress(unsigned char o1, unsigned char o2, unsigned char o3, unsigned char o4) 
:   InetAddress((unsigned long)MAKE_ADDR(o1, o2, o3, o4)) { }

InetAddress::InetAddress(unsigned long address) 
: _address(address) { }

InetAddress::InetAddress(const InetAddress& inetAddrRef) 
:   InetAddress(inetAddrRef._address) { }

InetAddress::InetAddress(in_addr address)
:   InetAddress(*(unsigned long*)&address) { }

unsigned long& InetAddress::address() {
    return this->_address;
}

InetAddress::operator unsigned long() const {
    return this->_address;
}

InetAddress::operator in_addr() const {
    return *(in_addr*)&this->_address;
}

InetAddress& InetAddress::operator=(const InetAddress& inetAddrRef) {
    this->_address = inetAddrRef._address;
    return *this;
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& stream, const InetAddress& inetAddrRef) {
    stream << inet_ntoa(inetAddrRef);
    return stream;
}

//
//
//
//
//

InetHost::InetHost() {
    this->_host.sin_family = AF_INET;
}

InetHost::InetHost(sockaddr_in host) 
:   _host(host) { }

InetHost::InetHost(sockaddr host) 
:   _host(*(sockaddr_in*)&host) { }

InetHost::InetHost(unsigned long address, unsigned short port) : InetHost() {
    *(unsigned long*)&this->_host.sin_addr = address;
    this->_host.sin_port = htons(port);
}

InetHost::InetHost(const InetAddress& address, unsigned short port)
:   InetHost((unsigned long)address, port) { }

InetHost::InetHost(const char* address, unsigned short port)
:   InetHost(InetAddress(address), port) { }

InetHost::InetHost(const std::string& address, unsigned port)
:   InetHost(InetAddress(address), port) { }

const sockaddr_in& InetHost::host() const {
    return this->_host;
}

sockaddr_in& InetHost::host() {
    return this->_host;
}

void InetHost::setPort(unsigned short port) {
    this->_host.sin_port = htons(port);
}

void InetHost::setAddress(const InetAddress& address) {
    this->_host.sin_addr = (in_addr)address;
}

unsigned short InetHost::getPort() const {
    return htons(this->_host.sin_port);
}

InetAddress InetHost::getAddress() const {
    return this->_host.sin_addr;
}

bool InetHost::isValid() const {
    return !*(unsigned long*)&this->_host.sin_addr && !this->_host.sin_family && !this->_host.sin_port;
}

InetHost::operator sockaddr_in() const {
    return this->_host;
}

InetHost::operator sockaddr() const {
    return *(sockaddr*)&this->_host;
}

InetHost& InetHost::operator=(const InetHost& inetHostRef) {
    this->_host = inetHostRef._host;
    return *this;
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& stream, const InetHost& inetHostRef) {
    stream.put('<');
    stream << inet_ntoa(inetHostRef._host.sin_addr);
    stream.put(':');
    stream << htons(inetHostRef._host.sin_port);
    stream.put('>');
    return stream;
}


//
//
//
//
//

bool Socket::__SOCKET_INIT__() {
#ifdef _WIN32
    WSADATA windowsSocketApiData;
    if (WSAStartup(MAKEWORD(2, 2), &windowsSocketApiData) != NO_ERROR) {
        throw std::runtime_error("Unable To Intialize Windows Socket API");
    }
#endif

    return true;
}

const bool Socket::__IS_SOCKET_INITD__ = Socket::__SOCKET_INIT__();

//
//
//

void Socket::outputSocketLastError(std::basic_ostream<char>& outStream) {
    outStream << socket_errno << ' ' << '|' << ' ';

#ifdef _WIN32
    char* errorMessageBuffer;

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        socket_errno,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&errorMessageBuffer,
        0,
        NULL
    );

    outStream << errorMessageBuffer;

    LocalFree(errorMessageBuffer);
#else
    outStream << std::strerror(socket_errno);
#endif
}

//
//
//

Socket::Socket(int af, int type, int protocol) {
    this->_socket = ::socket(af, type, protocol);

    if (this->_socket == SOCKET_INVALID_VALUE) {
        Socket::outputSocketLastError(std::cerr);
    }
}

Socket::Socket(const SocketType& socket, const InetHost& host) 
:   _socket(socket), _socketHost(host) { }

Socket::Socket(const Socket& socketReference) 
:   _socket(socketReference._socket), _socketHost(socketReference._socketHost) { }

//
//
//

bool Socket::bind() const {
    return this->bind(this->_socketHost);
}

bool Socket::bind(const InetHost& bindHost) const {
    return !::bind(
        this->_socket, (sockaddr*)&bindHost.host(), sizeof(sockaddr)
    );
}

bool Socket::bind(unsigned short port) {
    this->_socketHost.setPort(port);
    return this->bind();
}

bool Socket::listen(int backLog) const {
    return !::listen(
        this->_socket, backLog
    );
}

Socket Socket::accept() const {
    SocketType clientSocket;
    sockaddr clientAddr;
    socklen_t clientAddrLen = sizeof(sockaddr);

    clientSocket = ::accept(this->_socket, &clientAddr, &clientAddrLen);

    return Socket(clientSocket, clientAddr);
}

bool Socket::close() const {
#ifdef _WIN32
    // windows `uint_ptr` closing
    return !::closesocket(this->_socket);
#else
    // unix socket file descriptor closing
    return !::close(this->_socket);
#endif
}

bool Socket::isValid() const {
    return this->_socket != SOCKET_INVALID_VALUE;
}

InetHost Socket::getSockName() const {
    socklen_t sockAddrSz = sizeof(sockaddr);
    InetHost host(0ul, 0);

    ::getsockname(
        this->_socket, (sockaddr*)&host.host(), &sockAddrSz
    );

    return host;
}

//
//
//
//
//

SocketConnection::SocketConnection(int af, int type, int protocol)
:   Socket(af, type, protocol) { }

SocketConnection::SocketConnection(const Socket& socket)
:   Socket(socket) { }

//
//
//

int SocketConnection::sendTo(const char* data, int dataLen, const InetHost* to, int flags) const {
    return ::sendto(
        this->_socket, data, dataLen, flags, (to ? (sockaddr*)&to->host() : NULL), sizeof(sockaddr)
    );  
}

int SocketConnection::recvFrom(char* data, int dataLen, InetHost* from, int flags) const {
    socklen_t fromAddrLen = sizeof(sockaddr);

    return ::recvfrom(
        this->_socket, data, dataLen, flags, (from ? (sockaddr*)&from->host() : NULL), &fromAddrLen
    );
}

int SocketConnection::sendTo(const std::vector<char>& data, const InetHost* to, int flags) const {
    return this->sendTo(data.data(), (int)data.size(), to, flags);
}

std::vector<char> SocketConnection::recvFrom(InetHost* from, int flags) const {
    std::vector<char> buffer(0xFFFF);
    int recvdLength = this->recvFrom(buffer.data(), 0xFFFF, from,  flags);
    buffer.resize((recvdLength > 0) ? recvdLength : 0);
    return buffer;
}

int SocketConnection::sendStringTo(const std::string& string, const InetHost* to, int flags) const {
    return this->sendTo(string.c_str(), (int)string.size(), to, flags);
}

std::string SocketConnection::recvStringFrom(InetHost* from, int flags) const {
    std::string stringBuffer(0xFFFF, '\0');
    int recvdLen = this->recvFrom((char*)stringBuffer.c_str(), 0xFFFF, from, flags);
    stringBuffer.resize((recvdLen > 0) ? recvdLen : 0);
    return stringBuffer;
}

int SocketConnection::send(const char* data, int dataLen, int flags) const {
    return this->sendTo(data, dataLen, NULL, flags);
}

int SocketConnection::recv(char* data, int dataLen, int flags) const {
    return this->recvFrom(data, dataLen, NULL, flags);
}

int SocketConnection::send(const std::vector<char>& data, int flags) const {
    return this->sendTo(data.data(), (int)data.size(), NULL, flags);
}

std::vector<char> SocketConnection::recv(int flags) const {
    return this->recvFrom(NULL, flags);
}

int SocketConnection::sendString(const std::string& string, int flags) const {
    return this->sendStringTo(string, NULL, flags);
}

std::string SocketConnection::recvString(int flags) const {
    return this->recvStringFrom(NULL, flags);
}

bool SocketConnection::connect() const {
    return this->connect(this->_socketHost);
}

bool SocketConnection::connect(const InetHost& connectHost) const {
    return !::connect(
        this->_socket, (sockaddr*)&connectHost.host(), sizeof(sockaddr)
    );
}

//
//
//
//
//

TcpClient::TcpClient(const Socket& socket) 
:   SocketConnection(socket) { }

TcpClient::TcpClient(const InetHost& host) 
:   SocketConnection(AF_INET, SOCK_STREAM, IPPROTO_TCP) {
    this->_socketHost = host;
}

TcpClient::TcpClient(const InetAddress& address, unsigned short port) 
:   TcpClient(InetHost(address, port)) { }

//
//
//
//
//

TcpServer::TcpServer() 
:   Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) { 
    this->_socketHost.setAddress(INADDR_ANY);
}

TcpServer::TcpServer(unsigned short port) 
:   TcpServer() {
    this->_socketHost.setPort(port);
}

//
//
//

TcpClient TcpServer::accept() {
    return TcpClient(Socket::accept());
}