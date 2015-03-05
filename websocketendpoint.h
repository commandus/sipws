#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "connectionmetadata.h"

typedef std::map<int, ConnectionMetadata::ptr> ConnectionList;

class WebsocketEndpoint 
{
private:
	std::ostream *mOutput;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> mThread;
	ConnectionList mConnectionList;
	int mNextId; 
public:
	ConnectionMetadata::client mEndpoint;
	WebsocketEndpoint(std::ostream &output);
	~WebsocketEndpoint();
	int connect(std::string const & uri);
	void close(int id, websocketpp::close::status::value code, std::string reason);
	ConnectionMetadata::ptr getMetadata(int id) const;
};


