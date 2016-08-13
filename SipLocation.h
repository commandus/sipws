#ifndef SIPLOCATEION_H
#define SIPLOCATEION_H

#include <vector>
#include <time.h>
#include <json/json.h>
#include "sipwsUtil.h"

class SipLocation
{
public:
	TProto Proto;	// udp, tcp, ws
	TPrefix Prefix;	// sip, sips, ws, wss
	std::string Host;
	int Port;
	std::string Tag;
	std::string Line;
	std::string Rinstance;
	int Expire;
	time_t Registered;

	SipLocation();
	~SipLocation();
	SipLocation(const Json::Value &value);

	/**
	* Check is contact expired or is not available
	* @param since Date
	* @return true- expired
	*/
	bool isExpired(time_t since);
	Json::Value toJson();
};

typedef std::vector<SipLocation> SipLocations;

#endif