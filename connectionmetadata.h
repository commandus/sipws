#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

class ConnectionMetadata
{
private:
	int mId;
	websocketpp::connection_hdl mHdl;
	std::string mStatus;
	std::string mUri;
	std::string mServer;
	std::string mErrorReason;
	std::ostream *mOutput;
public:
	typedef websocketpp::client<websocketpp::config::asio_client> client;
	typedef websocketpp::lib::shared_ptr<ConnectionMetadata> ptr;
	typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

	ConnectionMetadata(int id, websocketpp::connection_hdl hdl, const std::string &uri, std::ostream *output);
	void onOpen(client * c, websocketpp::connection_hdl hdl);
	void onFail(client * c, websocketpp::connection_hdl hdl);
	void onClose(client * c, websocketpp::connection_hdl hdl);
	void onMessage(client* c, websocketpp::connection_hdl hdl, message_ptr msg);

	websocketpp::connection_hdl getHandler() const 
	{
		return mHdl;
	}

	int getId() const 
	{
		return mId;
	}

	std::string getStatus() const 
	{
		return mStatus;
	}

	friend std::ostream & operator<< (std::ostream & out, ConnectionMetadata const & data);
};


std::ostream & operator<< (std::ostream & out, ConnectionMetadata const & data);