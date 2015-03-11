#ifndef LOGGER_FUNC_H
#define	LOGGER_FUNC_H	1
#include <string>

typedef void(*TLoggerFunc)(time_t t, int command, int code, const std::string &idfrom, const std::string &idto, const std::string &msg);

TLoggerFunc loadLogger(const std::string &path, const std::string &name, const std::string &funcname, void **handle);
void closeLibrary(void *handle);

#endif