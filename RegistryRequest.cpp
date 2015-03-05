#include "RegistryRequest.h"

RegistryRequest::RegistryRequest() : q(commandnop), key(""), domain(""), v(0)
{
}

RegistryRequest::RegistryRequest(ECommand q, const std::string& domain, const std::string& key, size_t v, const std::vector<RegistryEntry> &entries)
{
	this->q = q;
	this->domain = domain;
	this->key = key;
	this->v = v;
	this->entries = entries;
}

RegistryRequest::RegistryRequest(const Json::Value &value)
{
	if ((&value == NULL) || (!value.isObject()))
	{
		this->q = commandnop;
		this->key = "";
		this->v = 0;
		return;
	}

	Json::Value o(value.get("q", ""));
	this->q = parseCommand(o.asString());
	this->domain = value.get("domain", "").asString();
	this->key = value.get("key", "").asString();
	this->v = parseSize_t(value.get("v", "0").asString());
	Json::Value v(value.get("data", Json::arrayValue));
	if ((&v != NULL) && v.isArray())
	{
		for (unsigned int i = 0; i < v.size(); i++)
		{
			this->entries.push_back(v[i]);
		}
	}
}

RegistryRequest::~RegistryRequest()
{
}

Json::Value RegistryRequest::toJson()
{
	Json::Value r(Json::objectValue);
	std::string s(toString(q));
	if (!s.empty())
		r["q"] = s;
	if (!domain.empty())
		r["domain"] = domain;

	if (&entries != NULL)
	{
		Json::Value d(Json::arrayValue);
		for (RegistryEntries::iterator it = entries.begin(); it != entries.end(); ++it)
		{
			d.append(it->toJson());
		}
		r["data"] = d;
	}
	r["v"] = (Json::Value::UInt64) v;
	return r;
}