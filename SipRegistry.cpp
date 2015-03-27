#include "SipRegistry.h"
#include <algorithm>

SipRegistry::SipRegistry() 
	: Domain(""), Ip(""), logger(NULL), Expires(DEF_EXPIRATION_SEC), dbFileName(""), version(0)
{
}

SipRegistry::SipRegistry(Logger *alogger, const std::string dbfn) 
	: Domain(""), Ip(""), logger(alogger), dbFileName(dbfn), Expires(DEF_EXPIRATION_SEC), version(0)
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

bool SipRegistry::exist(const std::string &key)
{
	return list.find(key) != list.end();
}

SipAddress *SipRegistry::get(const std::string &key)
{
	TSipAddressList::iterator it = list.find(key);
	if (it == list.end())
		return NULL;
	return &it->second;
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
SipAddress *SipRegistry::getByAddress(const std::string &address)
{
	SipAddress a(PROTO_UNKN, address, 0);
	return getById(a.Id, a.Domain);
}

/**
* Return registered SipAddress
* @param id like "102"
* @param domain like "192.168.7.10"
* @return null if not registered
* @see #getByAddress(String)
*/
SipAddress *SipRegistry::getById(const std::string &id, const std::string &domain)
{
	return get(id + "@" + domain);
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

	struct isold 
	{
		time_t since;
		isold(time_t d) : since(d) { };
		bool operator()(std::pair<const std::string, SipAddress> it) const
		{
			return it.second.isExpired(since);
		}
	};

	TSipAddressList::iterator it = list.begin();
	while ((it = std::find_if(it, list.end(), isold(d))) != list.end())
		list.erase(it++);
}
