#ifndef REGISTRYENTRY_H
#define REGISTRYENTRY_H		1

#include <string>
#include <vector>
#include "json/json.h"
#include "sipwsutil.h"

class RegistryEntry
{
public:
	EOperation o;
	std::string u;
	std::string k;
	std::string cn;
	std::string description;
	std::string image;
	RegistryEntry();
	RegistryEntry(EOperation o, const std::string &u, const std::string &k, const std::string &cn, const std::string &description, const std::string &image);
	RegistryEntry(const Json::Value &value);
	~RegistryEntry();
	Json::Value toJson();
};

typedef std::vector<RegistryEntry> RegistryEntries;

#endif