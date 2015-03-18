#include <string>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include "event2/event.h"
#include "sipws.h"
#include "consolelocale.h"
#include "WSListener.h"
#include "HTTPListener.h"
#include "RegistryRequest.h"
#include "RegistryResponse.h"
#include "SipRegistry.h"
#include "SipServer.h"
#include "Logger.h"
#include "Deamonize.h"
#include "argsplit.h"
#include "LoggerFunc.h"

#define MAXKEYS	10
const char* progname = "sipws";

void *loggerhandle;

ConfigWSListener config;
WSListener *wslistener = NULL;
HTTPListener *httplistener = NULL;
SipRegistry *sipRegistry; 
SipServer *sipserver = NULL;
Logger *logger;

Json::FastWriter writer;
Json::Reader reader;

void done()
{
	if (httplistener)
		httplistener->wait();
	wslistener->wait();
	if (config.startUdp)
		sipserver->wait(PROTO_UDP);
	if (config.startTcp)
		sipserver->wait(PROTO_TCP);
	delete sipserver;
	delete sipRegistry;
	if (config.severity)
		if (logger && logger->lout(VERB_DEBUG))
			*logger->lout(VERB_DEBUG) << "Done" << std::endl;
	delete logger;
}

void stopRequest()
{
	if (wslistener)
		wslistener->stop();
	if (httplistener)
		httplistener->stop();
	if (config.startUdp)
		sipserver->stop(PROTO_UDP);
	if (config.startTcp)
		sipserver->stop(PROTO_TCP);
}

void onSignal(int signal)
{
	stopRequest();
	done();
	exit(signal);
}

size_t putEntries(const RegistryRequest &r)
{
	size_t version = sipRegistry->setVersion(r.v);
	if (!r.domain.empty())
		sipRegistry->Domain = r.domain;
	for (RegistryEntries::const_iterator it = r.entries.begin(); it != r.entries.end(); ++it)
	{
		if (it->u.empty())
			continue;
		SipAddress a(PROTO_UNKN, r.domain, it->u, it->k, it->cn, it->description, it->image);
		sipRegistry->put(a);
	}
	return version;
}

bool isKeyValid(const std::string value)
{
	return std::find(config.keys.begin(), config.keys.end(), value) != config.keys.end();
}

std::string onPost(const std::string &path, const std::string &data)
{
	Json::Value v;
	RegistryRequest r;
	int rslt = 0;
	size_t version = 0;
	if (reader.parse(data, v))
	{
		r = RegistryRequest(v);
		if (isKeyValid(r.key))
		{
			switch (r.q)
			{
			case commandput:
				version = putEntries(r);
				break;
			case commandlist:
				// TODO: NOT SUPPORTED
				break;
			case commandstart:
				// TODO: NOT SUPPORTED
				break;
			case commandstop:
				break;
			case commandclear:
				sipRegistry->clear();
				break;
			default:
				break;
			}
		}
		else
			rslt = ERR_KEY;
	}
	else
	{
		if (logger && logger->lout(VERB_ERR))
			*logger->lout(VERB_ERR) << "Error load " << path << ": " << reader.getFormatedErrorMessages();
		rslt = 2;
	}
	// return writer.write(r.toJson());
	RegistryResponse resp(r.q, rslt, version);

	// add list on request
	if ((rslt == 0) && (r.q == commandlist))
		resp.registry = sipRegistry;
	resp.errorcode = rslt;
	return writer.write(resp.toJson());
}

std::string onGet(const std::string &path, const std::string &data)
{
	return writer.write(sipRegistry->toJson(false));
}

/*
	Parameters:
	argcFromCommandLine	argumemt count before merge with arguments from sipws.cfg
*/
int parseCmd(int argc, char* argv[], ConfigWSListener &config, int argcFromCommandLine)
{
	struct arg_str  *a_host = arg_str0("i", "interface", "<IP>", "host name or IP address. Default 0.0.0.0- all interfaces");
	struct arg_int	*a_port_control = arg_int0("r", "httpport", "<port>", "HTTP register control port number, e.g. 8082");
	struct arg_int	*a_port = arg_int0("p", "port", "<port>", "SIP WS port number, e.g. 8080");
	struct arg_int	*a_port_tls = arg_int0("s", "tlsport", "<port>", "SIP WSS port number. e.g. 443");
	struct arg_int	*a_port_udp = arg_int0("u", "udpport", "<port>", "SIP UDP port number. Disable UDP transport if not specified");
	struct arg_int	*a_port_tcp = arg_int0("t", "tcpport", "<port>", "SIP TCP port number. Disable TCP transport if not specified");
	struct arg_file *a_certificate_chain = arg_file0("c", "certificate", "<file>", "certificate file, default server.pem");
	struct arg_file *a_certificate_pk = arg_file0("k", "pk", "<file>", "private key file, default server.pem");
	struct arg_str  *a_tls_password = arg_str0("w", "password", "<key>", "certificate password, e.g. test");
	struct arg_file *a_data = arg_file0("d", "data", "<file>", "initial registry request JSON file");
	struct arg_file *a_dbfilename = arg_file0("b", "db", "<file>", "writeable database file name");
	struct arg_lit  *a_verbose = arg_litn("v", "verbose", 0, 3, "severity: -v: error, -vv: warning, -vvv: debug. Default fatal error only.");
	struct arg_lit  *a_foreground = arg_lit0("f", "foreground", "Do not start deamon");
	struct arg_lit  *a_skipconfig = arg_lit0(NULL, "skipconfig", "Skip reading argruments from sipws.cfg");
	struct arg_str  *a_locale = arg_str0("l", "locale", "<locale>", "e.g. russian_russia.1251, ru_RU.UTF-8");
#ifdef WIN32
	struct arg_lit  *a_locale_utf8 = arg_lit0("8", "utf8", "locale use UTF-8");
#endif
	struct arg_lit  *a_help = arg_lit0("h", "help", "print this help and exit");
	struct arg_str  *a_key = arg_strn(NULL, NULL, "<key>", 1, MAXKEYS, "update registry valid keys (-r)");
	struct arg_end  *a_end = arg_end(20);
	void* argtable[] = { a_host, a_port_control, a_port, a_port_tls, a_port_udp, a_port_tcp,
		a_certificate_chain, a_certificate_pk, a_tls_password, a_data, a_dbfilename,
		a_foreground, a_verbose, a_locale,
#ifdef WIN32
		a_locale_utf8,
#endif
		a_skipconfig,
		a_help, a_key, a_end };
	int nerrors;
	/* verify the argtable[] entries were allocated sucessfully */
	if (arg_nullcheck(argtable) != 0)
	{
		std::cerr << "insufficient memory" << std::endl;
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	/* Parse the command line as defined by argtable[] */
	nerrors = arg_parse(argc, argv, argtable);
	/* Re-read without arguments from config file */
	if (a_skipconfig->count > 0)
	{
		nerrors = arg_parse(argcFromCommandLine, argv, argtable);
	}

	/* special case: '--help' takes precedence over error reporting */
	if ((a_help->count > 0) || nerrors)
	{
		std::cout << "Usage " << progname << std::endl;
		arg_print_syntax(stdout, argtable, "\n");
		std::cout << "simple websocket SIP service" << std::endl;
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 0;
	}
	if (a_locale->count > 0)
	{
		bool utf8 = false;
#ifdef WIN32
		utf8 = a_locale_utf8->count > 0;
#endif
		ConsoleLocale(std::string(*a_locale->sval), utf8);
	}

	if (a_port_control->count > 0)
	{
		config.startControl = true;
		config.portControl = *a_port_control->ival;
	}

	if (a_port->count > 0)
	{
		config.startPlain = true;
		config.portPlain = *a_port->ival;
	}

	if (a_port_tls->count > 0)
	{
		config.startTLS = true;
		config.portTLS = *a_port_tls->ival;
		if (a_certificate_chain->count > 0)
			config.certificateChainFile = *a_certificate_chain->filename;
		if (a_certificate_pk->count > 0)
			config.certificatePKFile = *a_certificate_pk->filename;
		if (a_tls_password->count > 0)
			config.passwordTLS = *a_tls_password->sval;
	}

	if (a_dbfilename->count > 0)
		config.dbfilename = *a_dbfilename->filename;

	config.severity = a_verbose->count;

	if (a_port_udp->count > 0)
	{
		config.startUdp = true;
		config.portUdp = *a_port_udp->ival;
	}

	if (a_port_tcp->count > 0)
	{
		config.startTcp = true;
		config.portTcp = *a_port_tcp->ival;
	}

	for (int i = 0; i < a_key->count; i++)
	{
		config.keys.push_back(*a_key->sval);
	}

	if (a_host->count > 0)
	{
		config.host = *a_host->sval;
	}

	if (a_data->count > 0)
	{
		config.requestfilename = *a_data->filename;
	}
	config.deamonize = (a_foreground->count == 0);
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

void run()
{
	logger = new Logger(config.binpath, progname, config.severity, config.deamonize);
	SignalHandler sh(*logger->lout(VERB_FATAL), onSignal);

	sipRegistry = new SipRegistry(logger, config.dbfilename);
	if (!config.requestfilename.empty())
	{
		bool err = false;
		try
		{
			std::ifstream t(config.requestfilename);
			std::string data((std::istreambuf_iterator<char>(t)),
				std::istreambuf_iterator<char>());
			if (data.empty())
			{
				if (logger && logger->lout(VERB_ERR))
					*logger->lout(VERB_ERR) << "Can not read (or empty) file " << config.requestfilename;
				err = true;
			}
			else
			{
				std::string r(onPost(config.requestfilename, data));
				err = r.find("\"errorcode\":0") == std::string::npos;
			}

		}
		catch (...)
		{
			if (logger && logger->lout(VERB_ERR))
				*logger->lout(VERB_ERR) << "Error read " << config.requestfilename;
			err = true;
		}
		if (err)
		{
			delete sipRegistry;
			exit(2);
		}
	}
	sipserver = new SipServer(sipRegistry, logger, &config);
	wslistener = new WSListener(&config, logger, sipRegistry);
	sipserver->next = wslistener;
	wslistener->next = sipserver;
	if (config.startUdp)
		sipserver->start(PROTO_UDP, config.portUdp);
	if (config.startTcp)
		sipserver->start(PROTO_TCP, config.portTcp);
	wslistener->start();
	if (config.startControl)
	{
		httplistener = new HTTPListener(config.host, config.portControl, onPost, onGet);
		httplistener->start();
	}
}

int main(int argc, char* argv[])
{
	config.binpath = extractPath(argv[0]);
	std::string cmdline(loadCmdLine(config.binpath, "sipws.cfg"));
	int c;
	char **v;
	v = argsplit(cmdline, &c);
	v = argappend(&c, argc, argv, c, v);
	parseCmd(c, v, config, argc);
	config.cbLogger = loadLogger(config.binpath, "logger", "mylog", &loggerhandle);

	if (config.deamonize)
		Deamonize deamonize(progname, run, stopRequest, done);
	else
	{
		run();
		done();
	}

}

