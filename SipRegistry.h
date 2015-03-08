#ifndef SIP_REGISTRY_H
#define SIP_REGISTRY_H

#include <string>
#include <map>
#include "json/json.h"
#include "SipAddress.h"
#include "Logger.h"

typedef std::map<std::string, SipAddress> TSipAddressList;

class SipRegistry
{
private:
	Logger *logger;
	TSipAddressList list;
	size_t version;
	std::string dbFileName;
	bool load();
	bool load(const Json::Value &v);
	bool store();
protected:
	/**
	* Remove address from location database on unregister when "Expires" header equal 0.
	* Not used in application, instead set Expires to 0 so {@link SipLocation#clean()} remove it as expired.
	* @param id User identifier
	* @param domain SIP domain
	*/
	void rmAddress(const std::string &domain, const std::string &id);

public:
	time_t Expires;
	std::string Domain;
	// default IP address. If there more than 1 network interfaces, this address is used when SIP message routes from WS to UDP/TCP
	std::string Ip;
	SipRegistry();
	SipRegistry(Logger *logger, const std::string dbfn);
	~SipRegistry();
	size_t getVersion();
	size_t size();
	/* Return old version */
	size_t setVersion(size_t value);
	void clear();
	/**
	* Erase expired addresses.
	* It called from {@link ServiceCellTrack} thread as all others background tasks
	*/
	void clean();

	void put(SipAddress &value);
	/**
	* Add address to the location database
	* @param proto transport protocol
	* @param id user identifier
	* @param domain SIP domain
	* @param address InetAddress
	* @param port IP port number
	* @return true if it is my address!
	*/
	void put(TProto proto, const std::string &id, const std::string &domain, const std::string &tag, struct sockaddr_in *address, int expire, time_t registered);
	void put(TProto proto, const std::string &id, const std::string &domain, const std::string &tag, const std::string &host, int port, int expire, time_t registered);
	bool setAvailability(TProto proto, const std::string &id, const std::string &domain, TAvailability value);
	bool exist(const std::string &key);
	bool get(const std::string &key, SipAddress &value);

	/**
	* Return registered SipAddress
	* @param address	like "102" <sip:102@192.168.7.10>;tag=as533bb2a9
	* @return null if not registered
	* @see #getById(String, String)
	*/
	bool getByAddress(const std::string &address, SipAddress &result);

	/**
	* Return registered SipAddress
	* @param id like "102"
	* @param domain like "192.168.7.10"
	* @return null if not registered
	* @see #getByAddress(String)
	*/
	bool getById(const std::string &id, const std::string &domain, SipAddress &result);
	Json::Value toJson(bool deep);
};

#endif