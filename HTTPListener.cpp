#include <sstream>
#include <iostream>
#include <iomanip>

#include "HTTPListener.h"
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include "json/json.h"

static std::string readString(evbuffer *buf)
{
	std::ostringstream os;
	while (evbuffer_get_length(buf)) {
		int n;
		char cbuf[512];
		n = evbuffer_remove(buf, cbuf, sizeof(cbuf) - 1);
		if (n > 0)
		{
			os << std::string(cbuf, 0, n);
		}
	}
	return os.str();
}

static void dump_request_cb(struct evhttp_request *req, void *arg)
{
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;

	switch (evhttp_request_get_command(req)) {
		case EVHTTP_REQ_GET: cmdtype = "GET"; break;
		case EVHTTP_REQ_POST: cmdtype = "POST"; break;
		case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
		case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
		case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
		case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
		case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
		case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
		case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
		default: cmdtype = "unknown"; break;
	}
	std::ostringstream os;
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
	os << cmdtype << " "<< evhttp_request_get_uri(req) << std::endl;
	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
		header = header->next.tqe_next) {
		os << header->key << ": " << header->value << std::endl;
	}

	buf = evhttp_request_get_input_buffer(req);
	std::string data = readString(buf);
	evbuffer_add(buf, data.c_str(), data.size());
	evhttp_send_reply(req, 200, "OK", buf);
}

static void send_document_cb(struct evhttp_request *req, void *arg)
{
	HTTPListener* listener = (HTTPListener*) arg;
	const char *uri = evhttp_request_get_uri(req);

	if (listener == NULL)
	{
		dump_request_cb(req, arg);
		return;
	}

	/* Decode the URI */
	struct evhttp_uri *decoded = evhttp_uri_parse(uri);
	if (!decoded) 
	{
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}
	/* Let's see what path the user asked for. */
	std::string path(evhttp_uri_get_path(decoded));
	if (path.empty()) 
		path = "/";
	evhttp_uri_free(decoded);

	struct evbuffer *evb = evhttp_request_get_input_buffer(req);;
	std::string data = readString(evb);

	std::string reply;
	switch (evhttp_request_get_command(req))
	{
	case EVHTTP_REQ_GET:
		if (listener->onGet)
			reply = listener->onGet(path, data);
		break;
	case EVHTTP_REQ_POST:
			if (listener->onPost)
				reply = listener->onPost(path, data);
			break;
	default:
		evhttp_send_error(req, HTTP_BADMETHOD, 0);
		return;
	}

	evb = evbuffer_new();
	if (!evb)
	{
		evhttp_uri_free(decoded);
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/javascript");
	evbuffer_add(evb, reply.c_str(), reply.size());
	evhttp_send_reply(req, 200, "OK", evb);
	evbuffer_free(evb);
}

HTTPListener::HTTPListener(const std::string &address, const unsigned short port, OnMethod onpost, OnMethod onget)
{
	this->onPost = onpost;
	this->onGet = onget;
	listen(address, port);
}

HTTPListener::~HTTPListener()
{
}

int HTTPListener::start()
{
	return 0;
}

void HTTPListener::stop()
{
}

int HTTPListener::listen(const std::string &address, const unsigned short port)
{
	struct evhttp *http;
	struct evhttp_bound_socket *handle;
	base = event_base_new();
	if (!base) 
		return 1;
	
	// Create a new evhttp object to handle requests.
	http = evhttp_new(base);
	if (!http) 
		return 2;

	evhttp_set_gencb(http, send_document_cb, this);

	// Now we tell the evhttp what port to listen on
	handle = evhttp_bind_socket_with_handle(http, address.c_str(), port);
	if (!handle) 
		return 3;
	return 0;
}

void HTTPListener::wait()
{
	event_base_dispatch(base);
}
