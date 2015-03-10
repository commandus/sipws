#include "ConfigWSListener.h"

void ConfigWSListener::reset()
{
	cbLogger = NULL;
	deamonize = true;
	requestfilename = "";
	dbfilename = "";
	binpath = "";
	host = "0.0.0.0";
	severity = 0;
	backlog = 8192;
	startTLS = false;
	startPlain = false;
	startControl = false;
	startUdp = false;
	startTcp = false;
	portTLS = 443;
	portPlain = 8080;
	portControl = 8082;
	portUdp = 5060;
	portTcp = 5060;
	threads = 1;
	passwordTLS = "test";
	certificateChainFile = "server.pem";
	certificatePKFile = certificateChainFile;
	keys.clear();
}

ConfigWSListener::ConfigWSListener()
{
	reset();
}

ConfigWSListener::~ConfigWSListener()
{
}
