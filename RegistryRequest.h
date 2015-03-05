#ifndef REGISTRYREQUEST_H
#define REGISTRYREQUEST_H	1

#include <string>
#include "json/json.h"
#include "sipwsutil.h"
#include "RegistryEntry.h"

class RegistryRequest
{
public:
	ECommand q;
	std::string domain;
	std::string key;
	size_t v;
	RegistryEntries entries;

	RegistryRequest();
	RegistryRequest(ECommand q, const std::string& key, const std::string& domain, size_t v, const std::vector<RegistryEntry> &entries);
	RegistryRequest(const Json::Value &value);
	~RegistryRequest();
	Json::Value toJson();
};
#endif