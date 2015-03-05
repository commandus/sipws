#include "connectionmetadata.h"

ConnectionMetadata::ConnectionMetadata(int id, websocketpp::connection_hdl hdl, const std::string &uri, std::ostream *output)
	: mId(id), mHdl(hdl), mStatus("Connecting"), mUri(uri), mServer("N/A")
{
	mOutput = output;
}

void ConnectionMetadata::onOpen(client * c, websocketpp::connection_hdl hdl) 
{
	mStatus = "Open";

	client::connection_ptr con = c->get_con_from_hdl(hdl);
	mServer = con->get_response_header("Server");
}

void ConnectionMetadata::onFail(client * c, websocketpp::connection_hdl hdl) 
{
	mStatus = "Failed";

	client::connection_ptr con = c->get_con_from_hdl(hdl);
	mServer = con->get_response_header("Server");
	mErrorReason = con->get_ec().message();
}

void ConnectionMetadata::onClose(client * c, websocketpp::connection_hdl hdl) 
{
	mStatus = "Closed";
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	std::stringstream s;
	s << "close code: " << con->get_remote_close_code() << " ("
		<< websocketpp::close::status::get_string(con->get_remote_close_code())
		<< "), close reason: " << con->get_remote_close_reason();
	mErrorReason = s.str();
}

void ConnectionMetadata::onMessage(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
{
	ConnectionMetadata::client::connection_ptr con = c->get_con_from_hdl(hdl);
	// con->get_resource() msg->get_opcode()
	*mOutput << msg->get_payload();
	// c->send(hdl, msg->get_payload(), msg->get_opcode());
}

std::ostream & operator<< (std::ostream & out, ConnectionMetadata const & data) {
	out << "> URI: " << data.mUri << "\n"
		<< "> Status: " << data.mStatus << "\n"
		<< "> Remote Server: " << (data.mServer.empty() ? "None Specified" : data.mServer) << "\n"
		<< "> Error/close reason: " << (data.mErrorReason.empty() ? "N/A" : data.mErrorReason);

	return out;
}
