#include "SipServer.h"
#include "SipMessage.h"
#include <vector>
#ifdef WIN32
#include <winsock2.h>
typedef int32_t socklen_t;
#else
#define closesocket close
#endif
#include "sipwsUtil.h"

SipServer::SipServer(SipRegistry *registry, Logger *alogger, ConfigWSListener *aconfig) 
	: logger(alogger), config(aconfig), wslistener(NULL)
{
	for (int proto = 0; proto < PROTO_SIZE; proto++)
	{
		mSvcSocket[proto] = SOCKET_ERROR;
		memset(&mSvcAddress[proto], 0, sizeof(struct sockaddr_in));
		mSvcThread[proto] = NULL;
	}
	runner[0] = NULL;
	runner[1] = &SipServer::runUdp;
	runner[2] = &SipServer::runTcp;
	runner[3] = NULL;
	mDialog = new SipDialog(registry, logger);
	wslistener = new WSListener(config, logger, registry);
}

SipServer::~SipServer()
{
	delete wslistener;
	for (int proto = 1; proto < PROTO_SIZE; proto++)
		stop(proto);
}

// to avoid 10s lag
int setSocketOptions(int socket)
{
	int r = 0;
	/*

	int bufsize = 32768;
	struct timeval t0;
	t0.tv_sec = 0;
	t0.tv_usec = 0;

	r = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufsize, sizeof(bufsize));
	if (!r)
		r = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char *)&bufsize, sizeof(bufsize));

	// default
	if (!r)
		r = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&t0, sizeof(t0));
	if (!r)
		r = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&t0, sizeof(t0));
	*/
	return r;
}

bool SipServer::openSocket(int proto, int port)
{
	if (mSvcSocket[proto] != SOCKET_ERROR)
		return true;
	switch (proto)
	{
	case PROTO_UDP:
		if ((mSvcSocket[PROTO_UDP] = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
			return false;
		setSocketOptions(mSvcSocket[PROTO_UDP]);
		break;
	case PROTO_TCP:
		if ((mSvcSocket[PROTO_TCP] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == SOCKET_ERROR)
			return false;
		setSocketOptions(mSvcSocket[PROTO_TCP]);
		break;
	default:
		return false;
	}

	memset(&mSvcAddress[proto], 0, sizeof(struct sockaddr_in));
	mSvcAddress[proto].sin_family = AF_INET;
	mSvcAddress[proto].sin_port = htons(port);
	mSvcAddress[proto].sin_addr.s_addr = INADDR_ANY;

	if (bind(mSvcSocket[proto], (struct sockaddr*)&mSvcAddress[proto], sizeof(struct sockaddr)) < 0)
		return false;
	return true;
}

void SipServer::closeSocket(int proto)
{
	if (mSvcSocket[proto] == SOCKET_ERROR)
		return;
	closesocket(mSvcSocket[proto]);
	mSvcSocket[proto] = SOCKET_ERROR;
}


bool SipServer::start(int proto, int port)
{
	if (!openSocket(proto, port))
		return false;
	mSvcThread[proto] = new boost::thread(boost::bind(runner[proto], this));
	return true;
}

void SipServer::stop(int proto)
{
	closeSocket(proto);
	if (mSvcThread[proto] == NULL)
		return;
	mSvcThread[proto]->interrupt();
}

void SipServer::wait(int proto)
{
	mSvcThread[proto]->join();
	mSvcThread[proto] = NULL;
}

void SipServer::runUdp()
{
	while (true) {
		try
		{
			SipMessage msgIn;
			if (receive(PROTO_UDP, msgIn))
			{
				time_t now;
				time(&now);
				doProcessMessage(msgIn, now);
				boost::this_thread::interruption_point();
			}
		}
		catch (boost::thread_interrupted&)
		{
			return;
		}
		catch (...)
		{
			if (logger && logger->lout(VERB_ERR))
				*logger->lout(VERB_ERR) << "Error: UDP" << std::endl;

		}
	}
}

void SipServer::runTcp()
{
	while (true) {
		try
		{
			SipMessage msgIn;
			if (receive(PROTO_TCP, msgIn))
			{
				time_t now;
				time(&now);
				doProcessMessage(msgIn, now);
				boost::this_thread::interruption_point();
			}
		}
		catch (boost::thread_interrupted&)
		{
			return;
		}
		catch (...)
		{

		}
	}
}

#define RECV_BUF_SIZE	1024

/**
* Receive {@link SipMessage}
* @param socket
* @return {@link SipMessage} on true
*/
bool SipServer::receive(int proto, SipMessage &result)
{
	int socket = mSvcSocket[proto];
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 100;

	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	int rc = select(sizeof(fds) * 8, &fds, NULL, NULL, &timeout);
	if (rc > 0)
	{
		char rbuf[RECV_BUF_SIZE];
		struct sockaddr_in clientaddr;
		socklen_t addrlen = sizeof(clientaddr);
		int len;
		if ((len = recvfrom(socket, rbuf, RECV_BUF_SIZE, 0, (sockaddr*)&clientaddr, &addrlen)) > 0)
		{
			std::string msg = std::string(rbuf, len);
			// if (startsWith(msg, PING)) return false;
			if (msg.length() < 4)
				return false;	// jAK, CRLF
			if (!SipMessage::parse(msg, result))
				return false; 
			result.Proto = (TProto) proto;
			result.Address = clientaddr;

			if (result.mCommand == C_INVALID)
				return false; // client NAT ping
		}
	}
	return true;
}

/*
	Parameters:
		msgIn	received message 
		now		receiving time
*/
void SipServer::doProcessMessage(SipMessage &msgIn, time_t now)
{
	if ((&msgIn == NULL) || (msgIn.mCommand == C_INVALID))
		return;

	if (logger && logger->lout(VERB_DEBUG))
		*logger->lout(VERB_DEBUG) << "Received from " << addr2String(&msgIn.Address) << ":" << msgIn.Address.sin_port << std::endl
		<< msgIn.toString(true) << std::endl;

	std::vector<SipMessage> response;
	
	mDialog->mkResponse(&msgIn.Address, msgIn, response, now);
	
	for (SipMessage m : response) 
	{
		if (!sendMessage(m)) 
		{
			if (logger && logger->lout(VERB_WARN))
				*logger->lout(VERB_DEBUG) << "Error send " << std::endl
				<< m.toString(true) << std::endl;
		}
	}
}

/**
* Send message 
* @param m SipMessage
* @return true try other protocol
*/
bool SipServer::sendMessage(SipMessage &m) 
{
	if ((&m == NULL) || (m.sentCode & 1) || (m.mCommand == C_INVALID) || (m.Proto == PROTO_UNKN))
		return true;
	// avoid enless loop
	m.sentCode |= 1;
	int socket = mSvcSocket[m.Proto];
	if (socket == SOCKET_ERROR)
	{
		if ((next == NULL) || (!next->sendMessage(m)))
		{
			if (logger && logger->lout(VERB_WARN))
				*logger->lout(VERB_DEBUG) << "Error sip server send " << std::endl
				<< m.toString(true) << std::endl;
		}
		return false;
	}
	else
	{
		std::string s(m.toString(true));
		size_t sz = s.length();
		if (sz == 0)
			return true;
		if (logger && logger->lout(VERB_DEBUG))
			*logger->lout(VERB_DEBUG) << "Send to " << addr2String(&m.Address) << ":" << m.Address.sin_port << std::endl
			<< s << std::endl << std::endl;
		return sendto(socket, s.data(), sz, 0, (const sockaddr*)&m.Address, sizeof(struct sockaddr)) >= 0;
	}
		
}

