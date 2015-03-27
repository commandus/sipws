#ifndef SIP_ADDRESS_H
#define SIP_ADDRESS_H

#include <string>
#include <time.h>
#include <json/json.h>

#include "sipwsUtil.h"
#include "SipLocation.h"

class SipAddress
{
protected:
	static std::string getParam(const std::string &line, const std::string &name, int start);
	/* erase expired locations */
	void clean(const time_t dt);
public:
	TOrigin Origin;
	SipLocations locations;
	std::string Password;
	// name 
	std::string CommonName;
	// description
	std::string Description;
	std::string Id;
	std::string Domain;
	/**
	* update time
	*/
	time_t Updated;
	/**
	* User photo
	*/
	std::string Image;
	SipAddress();
	SipAddress(const SipAddress &value);
	SipAddress(const Json::Value &value);
	SipAddress(TProto proto, const std::string &domain, const std::string &id, const std::string &passwd, const std::string &cn, const std::string &description, const std::string &image);
	/*
	<sip:100@acme.com>;tag=11
	*/
	const std::string getAddressTag();
	
	/**
	* sip:103@192.168.7.1:5060;rinstance=987e406d3f7c951d
	* "102" <sip:102@192.168.7.10>;tag=as533bb2a9
	*/
	SipAddress(TProto defProto, const std::string &line, time_t now);
	~SipAddress();

	SipLocations::iterator getLocationIterator(const time_t since);
	SipLocations::iterator getLocationIterator(const SipLocation &search, const time_t since);
	/* if location does not exists, creates. */
	SipLocation *getLocation(time_t now);
	SipLocation &putLocation(const SipLocation &value, const time_t dt);
	bool rmLocation(const SipLocation *value, const time_t dt);

	const std::string getKey();
	/*
	sip:100@acme.com
	*/
	const std::string getAddress();

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