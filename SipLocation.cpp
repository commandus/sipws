#include "SipLocation.h"

SipLocation::SipLocation() :
	Proto(PROTO_UNKN), Prefix(PREFIX_UNKN), Host(""), Port(0), Tag(""), Line(""), Rinstance(""), Expire(0), Registered(0)
{
}

SipLocation::~SipLocation()
{
}

SipLocation::SipLocation(const Json::Value &value)
{
	Proto = parseProto(value["proto"].asString());
	Prefix = parsePrefix(value["prefix"].asString());
	Host = value["host"].asString();
	Port = value["port"].asInt();
	Line = value["line"].asString();
	Tag = value["tag"].asString();
	Rinstance = value["rinstance"].asString();
	Expire = value["expire"].asInt();
	Registered = value["registered"].asUInt64();
}

/**
* Check is contact expired or is not available
* @param since Date
* @return true- expired
*/
bool SipLocation::isExpired(time_t since)
{
	return (Registered + Expire) < since;
}

Json::Value SipLocation::toJson()
{
	Json::Value r(Json::objectValue);
	if (Proto != PROTO_UNKN)
		r["proto"] = toString(Proto);
	if (Prefix != PREFIX_UNKN)
		r["prefix"] = toString(Prefix);
	if ((&Host != NULL) && (!Host.empty()))
		r["host"] = Host;
	if (Port)
		r["port"] = Port;
	if ((&Line != NULL) && (!Line.empty()))
		r["line"] = Line;
	if ((&Tag != NULL) && (!Tag.empty()))
		r["tag"] = Tag;
	if ((&Rinstance != NULL) && (!Rinstance.empty()))
		r["rinstance"] = Rinstance;
	if (Expire)
		r["expire"] = Expire;
	if (Registered)
		r["registered"] = (Json::Value::UInt64) Registered;
	return r;
}

