#pragma once
#include <string>
#include <boost/thread.hpp>
#include "SipDialog.h"
#include "SipRegistry.h"
#include "Logger.h"
#include "ConfigWSListener.h"
#include "WSListener.h"
#include "SipListener.h"

class SipServer : public SipListener
{
	typedef void (SipServer::*runFunction) ();
private:
	Logger *logger;
	ConfigWSListener *config;
	WSListener *wslistener;
	struct sockaddr_in mSvcAddress[PROTO_SIZE];
	int mSvcSocket[PROTO_SIZE];
	boost::thread *mSvcThread[PROTO_SIZE];
	SipDialog *mDialog;
	void runUdp();
	void runTcp();
	runFunction runner[PROTO_SIZE];
protected:
	bool openSocket(int proto, int port);
	void closeSocket(int proto);
	/**
	* Receive {@link SipMessage}
	* @param socket
	* @return {@link SipMessage} on true
	*/
	bool receive(int proto, SipMessage &result);
	void doProcessMessage(SipMessage &msgIn, time_t now);
public:
	std::string Domain;
	SipServer(SipRegistry *registry, Logger *logger, ConfigWSListener *config);
	~SipServer();
	bool start(int proto, int port);
	void stop(int proto);
	void wait(int proto);
	virtual bool sendMessage(SipMessage &m);
};

