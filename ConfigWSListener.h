#ifndef _CONFIGWSLISTENER_H_
#define _CONFIGWSLISTENER_H_	1

#include <string>
#include <vector>
#include "LoggerFunc.h"

class ConfigWSListener
{
private:
	void reset();
public:
	std::string host;
	// initial request file
	std::string requestfilename;
	// json database file
	std::string dbfilename;
	// executable file path
	std::string binpath;
	int severity;
	int backlog;
	bool deamonize;
	bool startTLS;
	bool startPlain;
	bool startControl;
	bool startUdp;
	bool startTcp;
	int portTLS;
	int portPlain;
	int portControl;
	int portUdp;
	int portTcp;
	size_t threads;
	std::string passwordTLS;
	std::string certificateChainFile;
	std::string certificatePKFile;
	std::vector<std::string> keys;

	TLoggerFunc cbLogger;

	ConfigWSListener();
	~ConfigWSListener();
};

#endif