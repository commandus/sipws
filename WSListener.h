#ifndef _WSLISTENER_H_
#define _WSLISTENER_H_	1

#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "ConfigWSListener.h"
#include "websocketpp/config/asio.hpp"
#include "websocketpp/server.hpp"
#include "websocketpp/common/thread.hpp"

#include "json/json.h"
#include "Logger.h"
#include "SipDialog.h"
#include "SipRegistry.h"
#include "SipListener.h"

typedef websocketpp::server<websocketpp::config::asio> serverPlain;
typedef websocketpp::server<websocketpp::config::asio_tls> serverTLS;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_ptr;

class TextMessage
{
public:
	std::string subject;
	std::string body;
};

class WSListener;
class ConnectionState
{
private:
	bool isTLS;
public:
	std::string Key;
	websocketpp::connection_hdl h;
	WSListener *wslistener;
	ConnectionState() :h(), Key("")
	{
	};
	ConnectionState(WSListener *listener, websocketpp::connection_hdl handler, int64_t sid, bool tls) : wslistener(listener), h(handler), isTLS(tls), Key("") 
	{
	};
	bool isLogged() 
	{
		return !Key.empty();
	};
	bool send(const std::string &value);
};

typedef std::map<websocketpp::connection_hdl, ConnectionState> connectionList;
typedef std::map<std::string, websocketpp::connection_hdl> connectionUser;

class WSListener : public SipListener
{
private:
	Logger *logger;
	ConfigWSListener *config;
	SipDialog *mDialog;
	SipRegistry *registry;
	int64_t sessioncount;
	std::vector<thread_ptr> thread;
	connectionList mConnections;
	connectionUser mConnectionsUser;
	websocketpp::lib::mutex mActionLock;
	websocketpp::lib::mutex mConnectionLock;
	websocketpp::lib::condition_variable m_action_cond;
	void reset();
	bool getState(const websocketpp::connection_hdl hdl, ConnectionState &result);
	bool getState(const std::string &key, ConnectionState &result);
	// assign a key to the connection 
	void setStateKey(const websocketpp::connection_hdl hdl, const std::string &key);
	std::string getTLSPassword();
protected:
	void onSocketInit(websocketpp::connection_hdl, boost::asio::ip::tcp::socket & s);

	context_ptr onTLSInit(websocketpp::connection_hdl hdl);
	void onOpenPlain(websocketpp::connection_hdl hdl);
	void onOpenTLS(websocketpp::connection_hdl hdl);
	void onClose(websocketpp::connection_hdl hdl);
	void onMessagePlain(websocketpp::connection_hdl hdl, serverPlain::message_ptr msg);
	void onMessageTLS(websocketpp::connection_hdl hdl, serverTLS::message_ptr msg);
	bool listConnections(Json::Value &json);
	bool processMessage(websocketpp::connection_hdl &hdl, const serverPlain::message_ptr msg, std::string &ret);
public:
	serverPlain endpointPlain;
	serverTLS endpointTLS;

	WSListener();
	WSListener(ConfigWSListener *config, Logger *logger, SipRegistry *aregistry);
	~WSListener();
	size_t start();
	void wait();
	void stop();

	/**
	* Send message
	* @param m SipMessage
	* @return true try other protocol
	*/
	virtual bool sendMessage(SipMessage &m);
};

#endif