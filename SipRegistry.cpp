#include "SipRegistry.h"

SipRegistry::SipRegistry() 
	: Domain(""), Ip(""), logger(NULL), Expires(300), dbFileName(""), version(0)
{
}

SipRegistry::SipRegistry(Logger *alogger, const std::string dbfn) 
	: Domain(""), Ip(""), logger(alogger), dbFileName(dbfn), Expires(300), version(0)
{
	load();
}

SipRegistry::~SipRegistry()
{
	store();
}

size_t SipRegistry::getVersion()
{
	return version;
}

size_t SipRegistry::size()
{
	return list.size();
}

/* Return old version */
size_t SipRegistry::setVersion(size_t value)
{
	size_t o = version;
	version = value;
	return o;
}

void SipRegistry::clear()
{
	version = 0;
	list.clear();
}

void SipRegistry::put(SipAddress &value)
{
	list[value.getKey()] = value;
	if (logger && logger->lout(VERB_DEBUG))
		*logger->lout(VERB_DEBUG) << "Put " << value.toContact() << std::endl;
}

/**
* Add address to the location database
* @param proto transport protocol
* @param id user identifier
* @param domain SIP domain
* @param address InetAddress
* @param port IP port number
* @return true if it is my address!
*/
void SipRegistry::put(TProto proto, const std::string &id, const std::string &domain, const std::string &tag, struct sockaddr_in *address, int expire, time_t registered)
{
	put(proto, id, domain, tag, addr2String(address), address->sin_port, expire, registered);
}


/**
* Add address to the location database
* @param proto transport protocol
* @param id user identifier
* @param domain SIP domain
* @param address InetAddress
* @param port IP port number
* @return true if it is my address!
*/
void SipRegistry::put(TProto proto, const std::string &id, const std::string &domain, const std::string &tag, const std::string &host, int port,
	int expire, time_t registered)
{
	std::string key = id + "@" + domain;
	SipAddress a(list[key]);
	a.Availability = AVAIL_YES;
	if (proto != PROTO_UNKN)
		a.Proto = proto;
	if (!domain.empty())
		a.Domain = domain;
	if (!id.empty())
		a.Id = id;
	a.Tag = tag;
	a.Host = host;
	a.Port = port;
	a.Expire = (expire == 0 ? DEF_EXPIRES : expire);
	a.Registered = registered;
	list[key] = a;

	if (logger && logger->lout(VERB_DEBUG))
		*logger->lout(VERB_DEBUG) << "Update " << a.toContact() << std::endl;

}

bool SipRegistry::setAvailability(TProto proto, const std::string &id, const std::string &domain, TAvailability value)
{
	TSipAddressList::iterator it = list.find(id + "&" + domain);
	if (it == list.end())
		return false;
	it->second.Availability = value;
	return true;
}

bool SipRegistry::exist(const std::string &key)
{
	return list.find(key) != list.end();
}

bool SipRegistry::get(const std::string &key, SipAddress &value)
{
	TSipAddressList::iterator it = list.find(key);
	if (it == list.end())
		return false;
	value = it->second;
	return true;
}

bool SipRegistry::load(const Json::Value &value)
{
	Domain = value["domain"].asString();
	Ip = value["ip"].asString();
	if (Ip.empty())
		Ip = Domain;
	Json::Value v = value["data"];
	if (!v.isArray())
		return false;
	for (int i = 0; i < v.size(); i++)
	{
		SipAddress a(v[i]);
		put(a);
	}
	return true;
}

bool SipRegistry::load()
{
	if (dbFileName.empty())
		return false;
	try
	{
		std::string data(readFile(dbFileName));
		if (!data.empty())
		{
			Json::Reader reader;
			Json::Value v;
			if (reader.parse(data, v))
			{
				load(v);
				return true;
			}
			else
			{
				if (logger && logger->lout(VERB_WARN))
					*logger->lout(VERB_WARN) << "Error loading " << dbFileName 
					<< reader.getFormatedErrorMessages()
					<< std::endl;
				return false;
			}
		}
	}
	catch (...)
	{
		if (logger && logger->lout(VERB_WARN))
			*logger->lout(VERB_WARN) << "Error loading " << dbFileName << std::endl;

	}
	return false;
}

bool SipRegistry::store()
{
	if (dbFileName.empty())
		return false;
	Json::Value r = toJson(true);
	Json::FastWriter writer;
	try
	{
		std::string data(writer.write(r));
		if (!data.empty())
		{
			return writeFile(dbFileName, data) == 0;
		}
	}
	catch (...)
	{
	}
	return false;
}

Json::Value SipRegistry::toJson(bool deep)
{
	Json::Value retval(Json::objectValue);
	retval["domain"] = Domain;
	retval["ip"] = Ip;

	Json::Value r(Json::arrayValue);
	for (TSipAddressList::iterator it = list.begin(); it != list.end(); ++it)
	{
		r.append(it->second.toJson(deep));
	}
	retval["data"] = r;
	return retval;
}

/**
* Return registered SipAddress
* @param address	like "102" <sip:102@192.168.7.10>;tag=as533bb2a9
* @return null if not registered
* @see #getById(String, String)
*/
bool SipRegistry::getByAddress(const std::string &address, SipAddress &result)
{
	SipAddress a(PROTO_UNKN, address);
	return getById(a.Id, a.Host, result);
}

/**
* Return registered SipAddress
* @param id like "102"
* @param domain like "192.168.7.10"
* @return null if not registered
* @see #getByAddress(String)
*/
bool SipRegistry::getById(const std::string &id, const std::string &domain, SipAddress &result)
{
	return get(id + "@" + domain, result);
}

/**
* Remove address from location database on unregister when "Expires" header equal 0.
* Not used in application, instead set Expires to 0 so {@link SipLocation#clean()} remove it as expired.
* @param id User identifier
* @param domain SIP domain
*/
void SipRegistry::rmAddress(const std::string &domain, const std::string &id)
{
	try
	{
		list.erase(list.find(id + "@" + domain));
	}
	catch (...) {
		// Log.d(TAG, "Address remove " + uid + " error: " + e.toString());
	}
}

/**
* Erase expired addresses.
* It called from {@link ServiceCellTrack} thread as all others background tasks
*/
void SipRegistry::clean() 
{
	time_t d;
	time(&d);

	for (TSipAddressList::iterator it = list.begin(); it != list.end(); ++it) {
		if (it->second.isExpired(d))
				list.erase(it);
	}
}
