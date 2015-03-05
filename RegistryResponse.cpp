#include "RegistryResponse.h"


RegistryResponse::RegistryResponse() : q(commandnop), errorcode(0), v(0), registry(NULL)
{
}

RegistryResponse::RegistryResponse(ECommand q, int errorcode, size_t v)
{
	this->q = q;
	this->errorcode = errorcode;
	this->v = v;
	this->registry = NULL;
}

RegistryResponse::RegistryResponse(const Json::Value &value)
{
	if ((&value == NULL) || (!value.isObject()))
	{
		this->q = commandnop;
		this->errorcode = 0;
		this->v = 0;
		return;
	}

	Json::Value o(value.get("q", ""));
	this->q = parseCommand(o.asString());
	this->errorcode = parseInt(value.get("u", "0").asString());
	this->v = parseInt(value.get("k", "0").asString());
	this->registry = NULL;
}

RegistryResponse::~RegistryResponse()
{
}

Json::Value RegistryResponse::toJson()
{
	Json::Value r(Json::objectValue);
	std::string s(toString(q));
	if (!s.empty())
		r["q"] = s;
	r["errorcode"] = errorcode;
	r["v"] = (Json::Value::UInt64)v;
	if (registry)
		if (registry->size())
			r["registry"] = registry->toJson(false);
	return r;
}
