#include "websocketendpoint.h"

WebsocketEndpoint::WebsocketEndpoint(std::ostream &output) : mNextId(0)
{
	mOutput = &output;
	mEndpoint.clear_access_channels(websocketpp::log::alevel::all);
	mEndpoint.clear_error_channels(websocketpp::log::elevel::all);

	mEndpoint.init_asio();
	mEndpoint.start_perpetual();

	mThread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&ConnectionMetadata::client::run, &mEndpoint);
}

WebsocketEndpoint::~WebsocketEndpoint() {
	mEndpoint.stop_perpetual();

	for (ConnectionList::const_iterator it = mConnectionList.begin(); it != mConnectionList.end(); ++it) {
		if (it->second->getStatus() != "Open") 
		{
			// Only close open connections
			continue;
		}

		*mOutput << "> Closing connection " << it->second->getId() << std::endl;

		websocketpp::lib::error_code ec;
		mEndpoint.close(it->second->getHandler(), websocketpp::close::status::going_away, "", ec);
		if (ec) 
		{
			*mOutput << "> Error closing connection " << it->second->getId() << ": " << ec.message() << std::endl;
		}
	}

	mThread->join();
}

int WebsocketEndpoint::connect(std::string const & uri) {
	websocketpp::lib::error_code ec;

	ConnectionMetadata::client::connection_ptr con = mEndpoint.get_connection(uri, ec);

	if (ec) 
	{
		*mOutput << "> Connect initialization error: " << ec.message() << std::endl;
		return -1;
	}

	int new_id = mNextId++;
	ConnectionMetadata::ptr metadata_ptr = websocketpp::lib::make_shared<ConnectionMetadata>(new_id, con->get_handle(), uri, mOutput);
	mConnectionList[new_id] = metadata_ptr;

	con->set_open_handler(websocketpp::lib::bind(
		&ConnectionMetadata::onOpen,
		metadata_ptr,
		&mEndpoint,
		websocketpp::lib::placeholders::_1
		));
	con->set_fail_handler(websocketpp::lib::bind(
		&ConnectionMetadata::onFail,
		metadata_ptr,
		&mEndpoint,
		websocketpp::lib::placeholders::_1
		));
	con->set_close_handler(websocketpp::lib::bind(
		&ConnectionMetadata::onClose,
		metadata_ptr,
		&mEndpoint,
		websocketpp::lib::placeholders::_1
		));
	con->set_message_handler(websocketpp::lib::bind(
		&ConnectionMetadata::onMessage,
		metadata_ptr,
		&mEndpoint,
		websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2
		));
	mEndpoint.connect(con);

	return new_id;
}

void WebsocketEndpoint::close(int id, websocketpp::close::status::value code, std::string reason) {
	websocketpp::lib::error_code ec;

	ConnectionList::iterator metadata_it = mConnectionList.find(id);
	if (metadata_it == mConnectionList.end()) {
		*mOutput << "> No connection found with id " << id << std::endl;
		return;
	}

	mEndpoint.close(metadata_it->second->getHandler(), code, reason, ec);
	if (ec) 
	{
		*mOutput << "> Error initiating close: " << ec.message() << std::endl;
	}
}

ConnectionMetadata::ptr WebsocketEndpoint::getMetadata(int id) const {
	ConnectionList::const_iterator metadata_it = mConnectionList.find(id);
	if (metadata_it == mConnectionList.end()) {
		return ConnectionMetadata::ptr();
	}
	else {
		return metadata_it->second;
	}
}
