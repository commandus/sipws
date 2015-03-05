#ifndef LOGGER_H
#define LOGGER_H	1

#include <string>
#include <iostream>
#include <sstream>

#ifdef WIN32
#else
#include "syslog.h"
#endif

#define VERB_DEBUG	3	
#define VERB_WARN	2	
#define VERB_ERR	1
#define VERB_FATAL	0

class Logger
{
private:
	std::ostream *os;
	int severity;
	bool isDeamon;
#ifdef WIN32
#else
	syslog::ostream syslog;
#endif

public:
	Logger();
	Logger(const std::string &path, const std::string &logname, int verbosity, bool asDeamon);
	~Logger();
	void log(int severity, const std::string value);
	std::ostream *lout(int severity);
	int getSeverity();
	void setSeverity(int severity);
};

#endif