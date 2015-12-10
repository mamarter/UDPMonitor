#ifndef UDPSOCKET_CLASS_H
#define UDPSOCKET_CLASS_H

#include <winsock2.h>

class UDPSocket
{
public:

	UDPSocket(){}
	UDPSocket(unsigned short port);
	~UDPSocket();

	bool Initialise();
	bool OpenSocket();
	void CloseSocket();
	void CreatePollFd();

	SOCKET GetSocketFd() const { return mSocket; }
	WSAPOLLFD GetPollFd() const { return mPoll; }

private:

	WSAData mWsaData;
	WSAPOLLFD mPoll;
	unsigned short mPort;
	SOCKET mSocket;
};

#endif //UDPSOCKET_CLASS_H