#ifndef SIP_ADDRESS_H
#define SIP_ADDRESS_H

#include <string>
#include <time.h>
#include "json/json.h"

#include "sipwsUtil.h"

class SipAddress
{
protected:
	static std::string getParam(const std::string &line, const std::string &name, int start);
public:
	TAvailability Availability;
	TProto Proto;	// udp, tcp, ws
	TPrefix Prefix;	// sip, sips, ws, wss
	TOrigin Origin;
	std::string Password;
	std::string CommonName;
	std::string Description;
	std::string Id;
	std::string Domain;
	std::string Host;
	int Port;
	std::string Line;
	std::string Tag;
	std::string Rinstance;
	int Expire;
	time_t Registered;
	/**
	* Photo, name and description update time
	*/
	time_t Updated;
	/**
	* User photo
	*/
	std::string Image;
	SipAddress();
	SipAddress(const SipAddress &value);
	SipAddress(const Json::Value &value);
	SipAddress(TProto proto, const std::string &domain, const std::string &id, struct sockaddr_in *address, int port, int expire, TAvailability availability);
	SipAddress(TProto proto, const std::string &domain, const std::string &id, const std::string &passwd, const std::string &cn, const std::string &description, const std::string &image);
	/**
	* sip:103@192.168.7.1:5060;rinstance=987e406d3f7c951d
	* "102" <sip:102@192.168.7.10>;tag=as533bb2a9
	*/
	SipAddress(TProto defProto, const std::string &line);
	~SipAddress();

	const std::string getKey();
	Json::Value toJson(bool deep);
	
	/**
	* Extract Id from the string SIP representation
	* @param line
	* @return
	*/
	static std::string parseUid(const std::string &line);

	/**
	* Check is contact expired
	* @param since Date
	* @return true- expired
	*/
	bool isExpired(time_t since);

	/**
	* Return sip:100@192.168.1.1
	* @return
	*/
	std::string toRealm();

	/**
	* Return Contact ("M") formatted
	* @return
	*/
	std::string toContact();
};

#endif