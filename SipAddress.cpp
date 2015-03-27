#include "SipAddress.h"

#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>

SipAddress::SipAddress() :
	Origin(ORIGIN_NETWORK),
	Password(""),
	CommonName(""),
	Description(""),
	Id(""),
	Domain(""),
	Updated(0),
	Image("")
{

}

SipAddress::SipAddress(const SipAddress &value)
{
	this->Origin = value.Origin;
	this->Password = value.Password;
	this->CommonName = value.CommonName;
	this->Description = value.Description;
	this->Id = value.Id;
	this->Domain = value.Domain;
	this->Updated = value.Updated;
	this->Image = value.Image;

	for (SipLocations::const_iterator it = value.locations.begin(); it != value.locations.end(); ++it)
	{
		this->locations.push_back(*it);
	}
}

SipAddress::SipAddress(TProto proto, const std::string &domain, const std::string &id, const std::string &passwd, const std::string &cn, const std::string &description, const std::string &image) 
	:
	Origin(ORIGIN_REGISTRY),
	Updated(0),
	CommonName(cn),
	Description(description),
	Image(image),
	Id(id),
	Domain(domain),
	Password(passwd)
{

}

/**
* sip:103@192.168.7.1:5060;rinstance=987e406d3f7c951d
* "102" <sip:102@192.168.7.10>;tag=as533bb2a9
*/
SipAddress::SipAddress(TProto defProto, const std::string &line, time_t now) :
	Origin(ORIGIN_NETWORK),
	CommonName(""),
	Description(""),
	Image(""),
	Id(""),
	Domain(""),
	Password("")
{
	Updated = now;
	SipLocation location;
	location.Registered = now;

	if (&line == NULL) 
		return;

	// sip:, sips:, ws: wss:
	int pId = line.find(":");
	int pHost;
	location.Proto = defProto;
	location.Prefix = PREFIX_UNKN;
	if (pId < 0) {
		// try to parse w/o prefix
		Id = "";
		pHost = -1;
	}
	else
	{
		std::string pr(line.substr(0, pId));
		if (contains("sips", pr))
			location.Prefix = PREFIX_SIPS;
		else
			if (contains("sip", pr))
				location.Prefix = PREFIX_SIP;
			else
				if (contains("wss", pr))
					location.Prefix = PREFIX_WS;
				else
					if (contains("ws", pr))
						location.Prefix = PREFIX_WSS;

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
		for (unsigned int i = pPort + 1; i < line.length(); i++) {
			char c = line.at(i);
			if (!std::isdigit(c)) {
				pPortEnd = i;
				break;
			}
		}
		try 
		{
			location.Port = parseInt(line.substr(pPort, pPortEnd - pPort));
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
		Domain = line.substr(pHost + 1, pPort - pHost - 1);
		location.Host = Domain;
		location.Line = getParam(line, "line", pPort);
		location.Tag = getParam(line, "tag", pPort);
		location.Rinstance = getParam(line, "rinstance", pPort);

		std::string s = getParam(line, "proto", pPort);
		if (s.length() > 0)
			location.Proto = parseProto(s);

		s = getParam(line, "registered", pPort);
		if (s.length() > 0) {
			try 
			{
				location.Registered = parseTime_t(s);
			}
			catch (...)
			{
			}
		}

		s = getParam(line, "expire", pPort);
		if (s.length() > 0) {
			try 
			{
				location.Expire = parseInt(s);
			}
			catch (...) 
			{
			}
		}
	}
	catch (...) {
	}
	locations.push_back(location);
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
	SipLocations::iterator it = getLocationIterator(0);
	if ((it != locations.end()) && (!it->Tag.empty())) 
		s << ";tag=" << it->Tag;
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
	Origin = parseOrigin(value["origin"].asString());
	Password = value["password"].asString();
	CommonName = value["cn"].asString();
	Description = value["description"].asString();
	Id = value["id"].asString();
	Domain = value["domain"].asString();
	Updated = value["updated"].asUInt64();
	Image = value["image"].asString();

	Json::Value locs = value["locations"];
	if (locs.isArray())
	{
		for (unsigned int i = 0; i < locs.size(); ++i)
		{
			locations.push_back(SipLocation(locs[i]));
		}
	}
}

Json::Value SipAddress::toJson(bool deep)
{
	Json::Value r(Json::objectValue);
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
	if (Updated)
		r["updated"] = (Json::Value::UInt64) Updated;
	if ((&Image != NULL) && (!Image.empty()))
		r["image"] = Image;

	Json::Value locs(Json::arrayValue);
	for (SipLocations::iterator it = locations.begin(); it != locations.end(); ++it)
	{
		locs.append(it->toJson());
	}
	r["locations"] = locs;
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
	SipAddress a (PROTO_UNKN, line, 0);
	return a.Id;
}

/**
* Check is contact expired or is not available
* @param since Date
* @return true- expired
*/
bool SipAddress::isExpired(time_t since) 
{
	if (!&locations)
		return false;
	SipLocations::iterator r = getLocationIterator(since);
	return (r == locations.end());
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
	
	b << "<sip:" << Id << "@" << Domain;

	SipLocations::iterator l = getLocationIterator(0);
	if (l != locations.end())
	{
		b << ":" << l->Port;
		std::string p(toString(l->Proto));
		if (!p.empty())
			b << ";proto=" << p;
	}
	b << ">";
	if (l != locations.end())
	{
		if (!l->Tag.empty())
			b << ";tag=" << l->Tag;
	}
	return b.str();
}

/* erase expired locations */
void SipAddress::clean(const time_t dt)
{
	struct isold
	{
		time_t since;
		isold(time_t d) : since(d) { };
		bool operator()(SipLocation&it) const
		{
			return it.isExpired(since);
		}
	};
	SipLocations::iterator it = std::remove_if(locations.begin(), locations.end(), isold(dt));
	locations.erase(it, locations.end());
}

/*
	Parameters
		since	!= 0 delete expired since date time
*/
SipLocations::iterator SipAddress::getLocationIterator(const SipLocation &search, const time_t since)
{
	clean(since);

	struct issame
	{
		const SipLocation &search;
		issame(const SipLocation &s) : search(s) { };
		bool operator()(const SipLocation& it) const
		{
			return (((search.Proto == PROTO_UNKN) || ((it.Proto == search.Proto) && (it.Prefix == search.Prefix) 
				&& (it.Host == search.Host) && (it.Port == search.Port) && (it.Tag == search.Tag)))
				&& (search.Line.empty() || (it.Line == search.Line))
				&& (search.Rinstance.empty() || (it.Rinstance == search.Rinstance))
				);
		}
	};

	SipLocations::iterator it = locations.begin();
	return std::find_if(it, locations.end(), issame(search));
}

/*
	Return last updated or locations.end
*/
SipLocations::iterator SipAddress::getLocationIterator(const time_t since)
{
	SipLocation l;
	return getLocationIterator(l, since);
}

SipLocation *SipAddress::getLocation(time_t now)
{
	SipLocations::iterator it = getLocationIterator(now);
	if (it == locations.end())
	{
		SipLocation l;
		locations.push_back(l);
		it = locations.begin();
	}
	return &*it;
}

bool SipAddress::rmLocation(const SipLocation *value, const time_t dt)
{
	SipLocations::iterator it = getLocationIterator(*value, dt);
	if (it == locations.end())
		return false;
	locations.erase(it);
	return true;
}

SipLocation &SipAddress::putLocation(const SipLocation &value, const time_t dt)
{
	SipLocations::iterator it = getLocationIterator(value, dt);
	if (it != locations.end())
		locations.erase(it);
	locations.push_back(value);
	SipLocations::reverse_iterator rit = locations.rbegin();
	rit->Registered = dt;
	return *rit;
}

