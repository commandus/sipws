#include "Logger.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <streambuf>

#include <cctype>

Logger::Logger() 
	: severity(VERB_FATAL), os(&std::cerr), isDeamon(false)
{
}

Logger::Logger(const std::string &path, const std::string &logname, int aseverity, bool asDeamon) 
	: severity(aseverity), isDeamon(asDeamon)
{
	if (asDeamon)
	{
#ifdef WIN32
		os = new std::ofstream(path + logname + ".log", std::ofstream::out | std::ofstream::app);
		if ((!os) || os->fail())
			os = NULL;
#else
		// setlogmask(LOG_UPTO(LOG_NOTICE));
		// openlog(logname.c_str(), LOG_PID, LOG_DAEMON);
		os = &syslog;
#endif
	}
	else
		os = &std::cerr;
}

Logger::~Logger()
{
	if (os)
		os->flush();
#ifdef WIN32
	if (isDeamon)
	{
		((std::ofstream*)os)->close();
		delete os;
	}
#else
#endif
}

void Logger::log(int aseverity, const std::string value)
{
	if ((aseverity <= severity) && os)
		*os << value;
}

std::ostream *Logger::lout(int aseverity)
{
	if (aseverity <= severity)
		return os;
	else
		return NULL;
}

int Logger::getSeverity()
{
	return severity;
}

void Logger::setSeverity(int value)
{
	severity = value;
}

