#ifndef _SOCKET_WRAPPER_
#define _SOCKET_WRAPPER_

#pragma once


#include <iostream>
#include <vector>
#include <cerrno>
#include <cstring>


#ifdef _WIN32
#   define _WINSOCK_DEPRECATED_NO_WARNINGS
#   include <WinSock2.h>
#   include <WS2tcpip.h>
#   pragma comment(lib, "ws2_32")
#   define socket_errno WSAGetLastError()
#else
#   include <unistd.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   define socket_errno errno
#endif


#ifdef _WIN32
    typedef SOCKET SocketType;
#else
    typedef int SocketType;
#endif


#define MAKE_ADDR(_o1, _o2, _o3, _o4) (((_o4) << 24) | ((_o3) << 16) | ((_o2) << 8) | (_o1))
#define SOCKET_INVALID_VALUE ((SocketType)(~0u))

class InetAddress {
private:
    unsigned long _address;

public:
    InetAddress(const char* host);
    InetAddress(const std::string& host);
    InetAddress(unsigned char o1, unsigned char o2, unsigned char o3, unsigned char o4);
    InetAddress(const InetAddress& inetAddrRef);

    InetAddress(unsigned long address);
    InetAddress(in_addr address);

    unsigned long& address();

    // type cast conversions
    operator unsigned long() const;
    operator in_addr() const;

    InetAddress& operator=(const InetAddress& inetAddrRef);

    friend std::basic_ostream<char>& operator<<(std::basic_ostream<char>& stream, const InetAddress& inetAddrRef);
};

class InetHost {
private:
    sockaddr_in _host = { 0 };

public:
    InetHost();
    InetHost(sockaddr_in host);
    InetHost(sockaddr host);
    InetHost(unsigned long address, unsigned short port);
    InetHost(const InetAddress& address, unsigned short port);
    InetHost(const char* address, unsigned short port);
    InetHost(const std::string& address, unsigned port);

    void setPort(unsigned short port);
    void setAddress(const InetAddress& address);

    unsigned short getPort() const;
    InetAddress getAddress() const;

    const sockaddr_in& host() const;
    sockaddr_in& host();

    bool isValid() const;

    operator sockaddr_in() const;
    operator sockaddr() const;

    InetHost& operator=(const InetHost& inetHostRef);

    friend std::basic_ostream<char>& operator<<(std::basic_ostream<char>& stream, const InetHost& inetHostRef);
};

class Socket {
private:
    static bool __SOCKET_INIT__();
    static const bool __IS_SOCKET_INITD__;

public:
    static void outputSocketLastError(std::basic_ostream<char>& outStream);

protected:
    SocketType _socket = SOCKET_INVALID_VALUE;

    // note, _socketHost is not always the _socket's 'name',
    // it is the primary host address information the socket
    // will perform operations with.
    InetHost _socketHost;

    Socket(int af, int type, int protocol);
    Socket(const SocketType& socket, const InetHost& host);
    Socket(const Socket& socketReference);

public:

    template<typename OptionValueType>
    bool getSockOpt(int level, int option, OptionValueType& optionValue) {
        return !::getsockopt(
            this->_socket, level, option, (void*)&optionValue, sizeof(OptionValueType)
        );
    }

    template<typename OptionValueType>
    bool setSockOpt(int level, int option, const OptionValueType& optionValue) {
        return !::setsockopt(
            this->_socket, level, option, (void*)&optionValue, sizeof(OptionValueType)
        );
    }

    bool bind() const;
    bool bind(const InetHost& bindHost) const;
    bool bind(unsigned short port);
    bool listen(int backLog = SOMAXCONN) const;
    Socket accept() const;
    bool close() const;
    bool isValid() const;
    InetHost getSockName() const;

    Socket& operator=(const Socket& sref);
};

//
//
//

class SocketConnection : public Socket {
protected:
    SocketConnection(int af, int type, int protocol);
    SocketConnection(const Socket& socket);
    
public:

    bool connect() const;
    bool connect(const InetHost& connectHost) const;

    int sendTo(const char* data, int dataLen, const InetHost* to , int flags = 0) const;
    int recvFrom(char* data, int dataLen, InetHost* from, int flags = 0) const;

    int sendTo(const std::vector<char>& data, const InetHost* to = NULL, int flags = 0) const;
    std::vector<char> recvFrom(InetHost* from = NULL, int flags = 0) const;

    int sendStringTo(const std::string& string, const InetHost* to = NULL, int flags = 0) const;
    std::string recvStringFrom(InetHost* from = NULL, int flags = 0) const;

    int send(const char* data, int dataLen, int flags = 0) const;
    int recv(char* data, int dataLen, int flags = 0) const;

    int send(const std::vector<char>& data, int flags = 0) const;
    std::vector<char> recv(int flags = 0) const;

    int sendString(const std::string& string, int flags = 0) const;
    std::string recvString(int flags = 0) const;

    template<typename GenericType>
    bool send(const GenericType& typeValue, int flags = 0) const {
        return this->send((char*)&typeValue, sizeof(GenericType), flags) == sizeof(GenericType);
    }
    template<typename GenericType>
    bool recv(GenericType& typeValue, int flags = 0) const {
        char buffer[sizeof(GenericType)];
        if (this->recv(buffer, sizeof(GenericType), flags) != sizeof(GenericType)) {
            return false;
        }
        typeValue = *(GenericType*)buffer;
        return true;
    }

// deleted socket operations
public:
    bool bind() const = delete;
    bool bind(const InetAddress& bindHost) const = delete;
    bool listen(int backLog = SOMAXCONN) const = delete;
    Socket accept() const = delete;
};

//
//
//

class TcpClient : public SocketConnection {
public:
    TcpClient(const Socket& socket);
    TcpClient(const InetHost& host);
    TcpClient(const InetAddress& address, unsigned short port);
};

//
//
//

class TcpServer : public Socket {
public:
    TcpServer();
    TcpServer(unsigned short port);
    TcpClient accept();
};

#endif