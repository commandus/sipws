#include "SipAddress.h"

#include <iostream>
#include <sstream>
#include <cctype>

SipAddress::SipAddress() :
	Availability(AVAIL_NO),
	Origin(ORIGIN_NETWORK),
	Proto(PROTO_UNKN),
	Prefix(PREFIX_UNKN),
	Password(""),
	CommonName(""),
	Description(""),
	Id(""),
	Domain(""),
	Host(""),
	Port(0),
	Line(""),
	Tag(""),
	Rinstance(""),
	Expire(DEF_EXPIRES),
	Registered(0),
	Updated(0),
	Image("")
{

}

SipAddress::SipAddress(const SipAddress &value)
{
	this->Availability = value.Availability;
	this->Origin = value.Origin;
	this->Proto = value.Proto;
	this->Prefix = value.Prefix;
	this->Password = value.Password;
	this->CommonName = value.CommonName;
	this->Description = value.Description;
	this->Id = value.Id;
	this->Domain = value.Domain;
	this->Host = value.Host;
	this->Port = value.Port;
	this->Line = value.Line;
	this->Tag = value.Tag;
	this->Rinstance = value.Rinstance;
	this->Expire = value.Expire;
	this->Registered = value.Registered;
	this->Updated = value.Updated;
	this->Image = value.Image;
}

SipAddress::SipAddress(TProto proto, const std::string &domain, const std::string &id, struct sockaddr_in *address, int port, int expire, TAvailability availability)
	:
	Proto(proto),
	Prefix(PREFIX_UNKN),
	Origin(ORIGIN_NETWORK),
	Availability(availability),
	Port(port),
	CommonName(""),
	Description(""),
	Image(""),
	Id(id),
	Domain(domain),
	Rinstance(""),
	Tag(""),
	Line(""),
	Expire(expire),
	Password("")
{
	Host = addr2String(address);
	time(&Registered);
	Updated = Registered;
}

SipAddress::SipAddress(TProto proto, const std::string &domain, const std::string &id, const std::string &passwd, const std::string &cn, const std::string &description, const std::string &image)
	:
	Proto(proto),
	Prefix(PREFIX_UNKN),
	Origin(ORIGIN_REGISTRY),
	Availability(AVAIL_NO),
	Port(5060),
	Rinstance(""),
	Tag(""),
	Line(""),
	Registered(0),
	Updated(0),
	Expire(DEF_EXPIRES),	// 1 hour in seconds

	CommonName(cn),
	Description(description),
	Image(image),
	Id(id),
	Host(""),
	Domain(domain),
	Password(passwd)
{

}

/**
* sip:103@192.168.7.1:5060;rinstance=987e406d3f7c951d
* "102" <sip:102@192.168.7.10>;tag=as533bb2a9
*/
SipAddress::SipAddress(TProto defProto, const std::string &line) :
	Availability(AVAIL_NO),
	Origin(ORIGIN_NETWORK),
	Host(""),
	Port(0),
	CommonName(""),
	Description(""),
	Image(""),
	Id(""),
	Domain(""),
	Rinstance(""),
	Tag(""),
	Line(""),
	Expire(DEF_EXPIRES),	// 1 hour in seconds
	Password("")
{
	time(&Registered);
	Updated = Registered;

	if (&line == NULL) 
		return;

	// sip:, sips:, ws: wss:
	int pId = line.find(":");
	int pHost;
	Proto = defProto;
	Prefix = PREFIX_UNKN;
	if (pId < 0) {
		// try to parse w/o prefix
		Id = "";
		pHost = -1;
	}
	else
	{
		std::string pr(line.substr(0, pId));
		if (contains("sips", pr))
			Prefix = PREFIX_SIPS;
		else
			if (contains("sip", pr))
				Prefix = PREFIX_SIP;
			else
				if (contains("wss", pr))
					Prefix = PREFIX_WS;
				else
					if (contains("ws", pr))
						Prefix = PREFIX_WSS;

		pHost = line.find("@", pId + 1);
		if (pHost <= 0)
			return;
		/*
			sip:1@d
			0123456
			pid=3 phost=5
		*/
		Id = line.substr(pId + 1, pHost - pId - 1);
	}

	int pPort = line.find(":", pHost + 1);
	if (pPort <= 0)
		pPort = line.length();
	else {
		pPort++;
		int pPortEnd = line.length();
		for (int i = pPort + 1; i < line.length(); i++) {
			char c = line.at(i);
			if (!std::isdigit(c)) {
				pPortEnd = i;
				break;
			}
		}
		try 
		{
			Port = parseInt(line.substr(pPort, pPortEnd - pPort));
		}
		catch (...) 
		{
		}
	}
	try 
	{
		for (int i = pHost + 1; i < pPort; i++) {
			char c = line.at(i);
			if (!(isalnum(c) || (c == '.'))) {
				pPort = i;
				break;
			}
		}
		Host = line.substr(pHost + 1, pPort - pHost - 1);

		Line = getParam(line, "line", pPort);

		Tag = getParam(line, "tag", pPort);

		Rinstance = getParam(line, "rinstance", pPort);

		std::string s = getParam(line, "proto", pPort);
		if (s.length() > 0)
			Proto = parseProto(s);

		s = getParam(line, "registered", pPort);
		if (s.length() > 0) {
			try 
			{
				Registered = parseTime_t(s);
			}
			catch (...)
			{
			}
		}

		s = getParam(line, "expire", pPort);
		if (s.length() > 0) {
			try 
			{
				Expire = parseInt(s);
			}
			catch (...) 
			{
			}
		}
	}
	catch (...) {
	}
}


SipAddress::~SipAddress()
{
}

// get sockaddr, IPv4 or IPv6 
// *(in_addr *)get_in_addr(addr)
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

const std::string SipAddress::getKey()
{
	std::stringstream s;
	s << Id << "@" << Domain;
	return s.str();
}

/*
<sip:100@acme.com>;tag=11
*/
const std::string SipAddress::getAddressTag()
{
	std::stringstream s;
	s << "<sip:" << Id << "@" << Domain << ">";
	if (!Tag.empty()) 
		s << ";tag=" << Tag;
	return s.str();
}

/*
sip:100@acme.com
*/
const std::string SipAddress::getAddress()
{
	std::stringstream s;
	s << "sip:" << Id << "@" << Domain;
	return s.str();
}


SipAddress::SipAddress(const Json::Value &value)
{
	Availability = parseAvailability(value["avail"].asString());
	Proto = parseProto(value["proto"].asString());
	Prefix = parsePrefix(value["prefix"].asString());
	Origin = parseOrigin(value["origin"].asString());
	Password = value["password"].asString();
	CommonName = value["cn"].asString();
	Description = value["description"].asString();
	Id = value["id"].asString();
	Domain = value["domain"].asString();
	Host = value["host"].asString();
	Port = value["port"].asInt();
	Line = value["line"].asString();
	Tag = value["tag"].asString();
	Rinstance = value["rinstance"].asString();
	Expire = value["expire"].asInt();
	Registered = value["registered"].asUInt64();
	Updated = value["updated"].asUInt64();
	Image = value["image"].asString();
}

Json::Value SipAddress::toJson(bool deep)
{
	Json::Value r(Json::objectValue);
	r["avail"] = toString(Availability);
	if (Proto != PROTO_UNKN)
		r["proto"] = toString(Proto);
	if (Prefix != PREFIX_UNKN)
		r["prefix"] = toString(Prefix);
	r["origin"] = toString(Origin);
	if (deep)
	{
		if ((&Password != NULL) && (!Password.empty()))
			r["password"] = Password;
	}
	if ((&CommonName != NULL) && (!CommonName.empty()))
		r["cn"] = CommonName;
	if ((&Description != NULL) && (!Description.empty()))
		r["description"] = Description;
	if ((&Id != NULL) && (!Id.empty()))
		r["id"] = Id;
	if ((&Domain != NULL) && (!Domain.empty()))
		r["domain"] = Domain;
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
	if (Updated)
		r["updated"] = (Json::Value::UInt64) Updated;
	if ((&Image != NULL) && (!Image.empty()))
		r["image"] = Image;
	return r;
}

std::string SipAddress::getParam(const std::string &line, const std::string &name, int start)
{
	std::string r;
	int pLine = line.find(";" + name + "=", start);
	if (pLine > 0) {
		int len = name.length() + 2;
		int next = line.find(';', pLine + len);
		if (next <= 0)
			next = line.length();
		r = line.substr(pLine + len, next - pLine - len);
	}
	else {
		r = "";
	}
	return r;
}

/**
* Extract Id from the string SIP representation
* @param line
* @return
*/
std::string SipAddress::parseUid(const std::string &line)
{
	if (&line == NULL)
		return "";
	// no sip: , @ so it must be uid itself
	if ((line.find(':', 0) == std::string::npos) && (line.find('@', 0) == std::string::npos))
		return line;
	SipAddress a (PROTO_UNKN, line);
	return a.Id;
}

/**
* Check is contact expired or is not available
* @param since Date
* @return true- expired
*/
bool SipAddress::isExpired(time_t since) 
{
	return (Availability == AVAIL_NO) || (Registered + Expire) < since;
}

/**
* Return sip:100@192.168.1.1
* @return
*/
std::string SipAddress::toRealm()
{
	std::stringstream b;
	b << "sip:" << Id << "@" << Domain;
	return b.str();
}

/**
* Return Contact ("M") formatted
* @return
*/
std::string SipAddress::toContact()
{
	std::stringstream b;
	if (!CommonName.empty())
		b << "\"" << replaceChar(CommonName, '\"', ' ') << "\" ";
	b << "<sip:" << Id << "@" << Domain << ":" << Port;
	std::string p(toString(Proto));
	if (!p.empty())
		b << ";proto=" << p << ">";

	if (!Tag.empty())
		b << ";tag=" << Tag;
	return b.str();
}
