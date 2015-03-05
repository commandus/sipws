#ifndef REGISTRYRESPONSE_H
#define REGISTRYRESPONSE_H	1

#include <string>
#include "json/json.h"
#include "sipwsutil.h"
#include "SipRegistry.h"

class RegistryResponse
{
public:
	ECommand q;
	int errorcode;
	size_t v;
	SipRegistry *registry;
	RegistryResponse();
	RegistryResponse(ECommand q, int errorcode,	size_t v);
	RegistryResponse(const Json::Value &value);
	~RegistryResponse();
	Json::Value toJson();
};

#endif