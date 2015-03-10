#include <string>
#include "WSListener.h"
#include "SipMessage.h"

// alias some of the bind related functions as they are a bit long
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::connection_hdl;
using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

#define DEF_COUNT	100

bool ConnectionState::send(const std::string &value)
{
	try
	{
		if (isTLS)
			wslistener->endpointTLS.send(h, value, websocketpp::frame::opcode::text);
		else
			wslistener->endpointPlain.send(h, value, websocketpp::frame::opcode::text);
		return true;
	}
	catch (...)
	{
	}
	return false;
}

void WSListener::reset()
{
	mDialog = new SipDialog(registry, logger);
	sessioncount = 0L;
}

WSListener::WSListener() : logger(NULL), registry(NULL)
{
	reset();
}

WSListener::WSListener(ConfigWSListener *aconfig, Logger *alogger, SipRegistry *aregistry) 
	: config(aconfig), logger(alogger), registry(aregistry)
{
	reset();
}

WSListener::~WSListener()
{
	delete mDialog;
}

void WSListener::onSocketInit(websocketpp::connection_hdl handler, boost::asio::ip::tcp::socket & s) 
{
	// boost::asio::ip::tcp::no_delay option(true);
	// s.set_option(option);
	// serverPlain::connection_ptr con = endpointPlain.get_con_from_hdl(handler);
	// std::cerr << s.local_endpoint().address();
}

void WSListener::onOpenPlain(websocketpp::connection_hdl handler) {
	sessioncount++;
	mConnections.insert(std::make_pair(handler, ConnectionState(this, handler, sessioncount, false)));
	// std::cerr << endpointPlain.get_con_from_hdl(handler)->get_socket().local_endpoint().address();
}

void WSListener::onOpenTLS(websocketpp::connection_hdl hdl) {
	sessioncount++;
	mConnections.insert(std::make_pair(hdl, ConnectionState(this, hdl, sessioncount, true)));
}

void WSListener::onClose(websocketpp::connection_hdl hdl) {
	int64_t id = 0L;
	try
	{
	}
	catch (...) {
	}
	//
	mConnections.erase(hdl);
}

bool WSListener::getState(const websocketpp::connection_hdl hdl, ConnectionState &result)
{
	connectionList::iterator it = mConnections.find(hdl);
	if (it == mConnections.end()) 
		return false;
	result = it->second;
	return true;
}

bool WSListener::getState(const std::string &key, ConnectionState &result)
{
	connectionUser::iterator u = mConnectionsUser.find(key);
	if (u == mConnectionsUser.end())
		return false;
	connectionList::iterator it = mConnections.find(u->second);
	if (it == mConnections.end()) 
		return false;
	result = it->second;
	return true;
}

// assign a key to the connection 
void WSListener::setStateKey(const websocketpp::connection_hdl hdl, const std::string &key)
{
	mConnectionsUser[key] = hdl;
	mConnections[hdl].Key = key;
}


bool WSListener::listConnections(Json::Value &json)
{
	bool r = false;
	Json::Value a;

	try
	{
		for (connectionList::iterator c(mConnections.begin()); c != mConnections.end(); ++c)
		{
			// a.append(jsonSerialize(c->second, true));
		}
		json["connections"] = a;
		r = true;
	}
	catch (...)
	{
	}
	return r;
}

bool WSListener::processMessage(websocketpp::connection_hdl &hdl, const serverPlain::message_ptr message, std::string &ret)
{
	ConnectionState connectionstate;
	ret =  "";
	try
	{
		connectionstate = mConnections[hdl];
	}
	catch (...) 
	{
		return false;
	}
	std::string msg = message->get_payload();
	
	// ...
	if (msg.length() < 4)
		return false;	// jAK, CRLF
	
	SipMessage req;
	if (!SipMessage::parse(msg, req))
		return false;
	if (req.mCommand == C_INVALID)
		return false; 
	req.Proto = PROTO_WS;
	req.Key = connectionstate.Key;
	//	or 	SipAddress a; SipAddress::parseUid(req.Headers["F"]) + "@" + registry->Domain;
	// if (connectionstate.Key.empty()) req.Key = SipAddress::parseUid(req.Headers["F"] + "@" + registry->Domain);

	time_t now;
	time(&now);
	
	if (logger && logger->lout(VERB_DEBUG))
		*logger->lout(VERB_DEBUG) << "Received from ws:" << req.Key << std::endl
		<< req.toString(true) << std::endl;

	std::vector<SipMessage> responses;

	mDialog->mkResponse(&req.Address, req, responses, now);

	for (SipMessage m : responses)
	{
		if ((req.mCommand == C_REGISTER) && (m.mCommand == C_RESPONSE) && (m.mCode == OK))
		{
			// REGISTER OK, update connection key
			std::string key(SipAddress::parseUid(req.Headers["F"]) + "@" + registry->Domain);
			setStateKey(hdl, key);
		}
		m.conn = &hdl;
		if (!sendMessage(m))
		{
			if ((next == NULL) || (!next->sendMessage(m)))
			{
				if (logger && logger->lout(VERB_WARN))
					*logger->lout(VERB_DEBUG) << "Error ws server send " << std::endl
					<< m.toString(true) << std::endl;
				return false;
			}
		}
	}
	return true;
}

/**
* Send message
* @param m SipMessage
* @return true try other protocol
*/
bool WSListener::sendMessage(SipMessage &m)
{
	if ((&m == NULL) || (m.sentCode & 2) || (m.mCommand == C_INVALID) || (m.Proto == PROTO_UNKN))
		return true;
	// avoid enless loop
	m.sentCode |= 2;
	if (m.Proto != PROTO_WS)
		return false;
	std::string s(m.toString(true));
	size_t sz = s.length();
	if (sz == 0)
		return true;
	if (logger && logger->lout(VERB_DEBUG))
		*logger->lout(VERB_DEBUG) << "Send to ws:" << m.Key << std::endl
		<< s << std::endl << std::endl;

	ConnectionState connectionstate;

	try
	{
		if (config->cbLogger)
			config->cbLogger(m.mCommand, m.mCode, m.KeyFrom, m.Key, m.mCommand == C_MESSAGE ? m.Sdp : "");

		if (m.Key.empty())
		{
			if (m.conn)
				return mConnections[*(websocketpp::connection_hdl*)m.conn].send(s);
		}
		else
		{
			if (getState(m.Key, connectionstate))
				return connectionstate.send(s);
		}
	}
	catch (...)
	{

	}
	return false;
}

void WSListener::onMessagePlain(websocketpp::connection_hdl hdl, serverPlain::message_ptr msg)
{
	std::string v;
	if (!processMessage(hdl, msg, v))
		return;
	// ConnectionState *cs; getState(hdl, cs);
	try 
	{
		endpointPlain.send(hdl, v, msg->get_opcode());
	}
	catch (const websocketpp::lib::error_code&) 
	{
	}
}

void WSListener::onMessageTLS(websocketpp::connection_hdl hdl, serverPlain::message_ptr msg)
{
	std::string v;
	if (!processMessage(hdl, msg, v))
		return;
	// ConnectionState *cs; getState(hdl, cs);
	try
	{
		endpointTLS.send(hdl, v, msg->get_opcode());
	}
	catch (const websocketpp::lib::error_code&) 
	{
	}
}

// No change to TLS init methods from echo_server_tls
std::string WSListener::getTLSPassword()
{
	return config->passwordTLS;
}

context_ptr WSListener::onTLSInit(websocketpp::connection_hdl hdl)
{
	context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));

	try 
	{
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::single_dh_use);
		ctx->set_password_callback(boost::bind(&WSListener::getTLSPassword, this));
		ctx->use_certificate_chain_file(config->certificateChainFile);
		ctx->use_private_key_file(config->certificatePKFile, boost::asio::ssl::context::pem);
	}
	catch (std::exception&) {
	}
	return ctx;
}

bool validateHandlerPlain(serverPlain& s, websocketpp::connection_hdl hdl)
{
	serverPlain::connection_ptr con = s.get_con_from_hdl(hdl);
	const std::vector<std::string> & subp_requests = con->get_requested_subprotocols();
	if (subp_requests.size() > 0)
		con->select_subprotocol(subp_requests[0]);
	return true;
}

bool validateHandlerTLS(serverTLS& s, websocketpp::connection_hdl hdl)
{
	serverTLS::connection_ptr con = s.get_con_from_hdl(hdl);
	const std::vector<std::string> & subp_requests = con->get_requested_subprotocols();
	if (subp_requests.size() > 0)
		con->select_subprotocol(subp_requests[0]);
	return true;
}

size_t WSListener::start()
{
	websocketpp::log::level al;
	websocketpp::log::level el;
	switch (config->severity)
	{
	case 0:
		// Total silence
		al = websocketpp::log::alevel::none;
		el = websocketpp::log::elevel::none;
		break;
	case 1:
		al = websocketpp::log::alevel::none; // message_header - 1;
		el = websocketpp::log::elevel::fatal - 1;
		break;
	case 2:
		al = websocketpp::log::alevel::none; // devel - 1;
		el = websocketpp::log::elevel::fatal - 1;
		break;
	default:
		al = websocketpp::log::alevel::none; // all
		el = websocketpp::log::elevel::all;
	}
	if (config->startPlain)
	{
		endpointPlain.set_access_channels(al);
		endpointPlain.set_error_channels(el);

		// Initialize ASIO
		endpointPlain.init_asio();
		endpointPlain.set_reuse_addr(true);

		endpointPlain.set_validate_handler(bind(&validateHandlerPlain, boost::ref(endpointPlain), ::_1));

		// Register our message handler
		endpointPlain.set_open_handler(boost::bind(&WSListener::onOpenPlain, this, ::_1));
		endpointPlain.set_close_handler(boost::bind(&WSListener::onClose, this, ::_1));
		endpointPlain.set_message_handler(boost::bind(&WSListener::onMessagePlain, this, ::_1, ::_2));
		endpointPlain.set_socket_init_handler(boost::bind(&WSListener::onSocketInit, this, ::_1, ::_2));
		
		// Listen on specified port with extended listen backlog
		endpointPlain.set_listen_backlog(config->backlog);
		endpointPlain.listen(config->portPlain);
		// Start the server accept loop
		endpointPlain.start_accept();
	}
	
	if (config->startTLS)
	{
		endpointPlain.set_access_channels(al);
		endpointPlain.set_error_channels(el);

		// Initialize ASIO
		endpointTLS.init_asio();
		endpointPlain.set_reuse_addr(true);
		
		endpointTLS.set_validate_handler(bind(&validateHandlerTLS, boost::ref(endpointTLS), ::_1));

		// Register our message handler
		endpointTLS.set_open_handler(boost::bind(&WSListener::onOpenTLS, this, ::_1));
		endpointTLS.set_close_handler(boost::bind(&WSListener::onClose, this, ::_1));
		endpointTLS.set_message_handler(boost::bind(&WSListener::onMessageTLS, this, ::_1, ::_2));
		endpointPlain.set_socket_init_handler(boost::bind(&WSListener::onSocketInit, this, ::_1, ::_2));
		// TLS endpoint has an extra handler for the tls init
		endpointTLS.set_tls_init_handler(boost::bind(&WSListener::onTLSInit, this, ::_1));
		// Listen on specified port with extended listen backlog
		endpointTLS.set_listen_backlog(config->backlog);
		endpointTLS.listen(config->portTLS);
		// Start the server accept loop
		endpointTLS.start_accept();
	}

	// Start the ASIO io_service run loop
	for (size_t i = 0; i < config->threads; i++) 
	{
		if (config->startPlain)
			thread.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>(&serverPlain::run,	&endpointPlain));
		if (config->startTLS)
			thread.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>(&serverTLS::run, &endpointTLS));
	}
	return thread.size();
}

void WSListener::stop()
{
	if (config->startPlain)
		endpointPlain.stop();
	if (config->startTLS)
		endpointTLS.stop();
}

void WSListener::wait()
{
	for (size_t i = 0; i < thread.size(); i++) 
		thread[i]->join();
}
