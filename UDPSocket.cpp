
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include "UDPSocket.h"

using namespace std;

UDPSocket::UDPSocket(unsigned short port)
	: mPort(port)
{
}

UDPSocket::~UDPSocket()
{
	CloseSocket();
}

bool UDPSocket::Initialise()
{
	if (WSAStartup(MAKEWORD(2, 2), &mWsaData) != 0)
	{
		cerr << "ERROR at UDPSocket::Initialise() - ";
		cerr << "WSAStartup error: " << WSAGetLastError() << endl;
		return false;
	}

	if (!OpenSocket())
	{
		return false;
	}
	else
	{
		CreatePollFd();
	}
	return true;
}

void UDPSocket::CloseSocket()
{
	closesocket(mSocket);
	WSACleanup();
}

bool UDPSocket::OpenSocket()
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;

	struct addrinfo* res = 0;
	int status;
	if ((status = getaddrinfo("localhost",
		to_string(mPort).c_str(), &hints, &res)) != 0)
	{
		cerr << "ERROR at UDPSocket::OpenSocket() - getaddrinfo() failed: ";
		cerr << WSAGetLastError() << endl;
		return false;
	}

	if ((mSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
	{
		cerr << "ERROR at UDPSocket::OpenSocket() - socket() failed: ";
		cerr << WSAGetLastError() << endl;
		return false;
	}

	if (status = bind(mSocket, res->ai_addr, res->ai_addrlen) < 0)
	{
		const int wsaError = WSAGetLastError();
		if (wsaError == WSAEADDRINUSE)
		{
			cerr << "ERROR at UDPSocket::OpenSocket() - bind(): ";
			cerr << "Address already in use at port " << mPort << endl;
		}
		else
		{
			cerr << "ERROR at UDPSocket::OpenSocket() - bind() failed: ";
			cerr << wsaError << endl;
		}
		return false;
	}

	return true;
}

void UDPSocket::CreatePollFd()
{
	mPoll.fd = mSocket;
	mPoll.events = POLLIN;
}





