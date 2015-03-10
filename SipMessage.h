#ifndef SIPMESSAGE_H
#define SIPMESSAGE_H	1

#include <string>
#include <map>
#include <utility>
#include "sipwsUtil.h"

class SipMessage
{
private:
	bool posHeaderParameter(const std::string &header, const std::string &parameter, std::pair<int, int> &ret);
	static std::string getHeaders(SipMessage *m, bool fullheader);
public:
	// avoid enless loop: 1 set by SipServer::sendMessage(), 2- WSListener::sendMessage()
	int sentCode;
	std::map<std::string, std::string> Headers;
	std::string Sdp;
	int mCommand;
	int mCode;
	std::string mCommandParam;
	TProto Proto;
	// destination address
	struct sockaddr_in Address;
	// WS/WSS user@domain
	std::string Key;
	std::string KeyFrom;
	void *conn;

	SipMessage();
	SipMessage(const SipMessage &m);
	SipMessage(TProto proto, struct sockaddr_in *svcaddr, struct sockaddr_in *sender, const SipMessage &request);
	~SipMessage();

	void setPort(int value);
	/**
	Add parameter to the header
	*/
	void addHeaderParameter(const std::string &header, const std::string &parameter, const std::string &value);
	void rmHeaderParameter(const std::string &header, const std::string &parameter);
	void rmHeader(const std::string &header);
	std::string getHeaderParameter(const std::string &header, const std::string &parameter);
	bool existHeaderParameter(const std::string &header, const std::string &parameter);
	void replaceHeaderParameter(const std::string &header, const std::string &parameter, const std::string &value);
	std::string getHeaders();
	std::string toString();
	std::string toString(bool fullheader);

	/**
	* Return "Expires" tag value
	* @return expiration period in seconds
	*/
	int getExpires();

	/**
	* Return true if "Expires" tag equals 0
	* @return
	*/
	bool isExpired();
	
	/**
	* Return true if it is message packet for client at specified IP address
	* @param from Internet address
	* @return true if it is message packet for client at specified IP address
	*/
	bool isMessageFor(struct sockaddr_in &from);

	/**
	* Return Max-Forwards value
	* @return Max-Forwards value
	*/
	int getMaxForwards();

	/**
	* Set Max-Forwards header
	* @param value Max-Forwards value
	*/
	void setMaxForwards(int value);

	/**
	* Extract port number from "V" (Via)
	* V: SIP/2.0/UDP 192.168.1.37:32867;branch=z9hG4bK8b5cb1ad0affaa4ee99ac5f70c0e3647333632;rport=32867;received=192.168.1.37
	* @param defaultValue
	* @return port number
	*/
	int getViaPort(int defaultValue);
	
	static bool parse(const std::string message, SipMessage &result);
};

#endif