#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H	1

#include <string>
#include "RegistryResponse.h"

typedef std::string (*OnMethod) (const std::string &path, const std::string &data);

class HTTPListener
{
private:
	struct event_base *base;
protected:
	/*
	Errors:
	1- fprintf(stderr, "Couldn't create an event_base\n");
	2- fprintf(stderr, "couldn't create evhttp\n");
	3- fprintf(stderr, "couldn't bind to port\n");
	*/
	int listen(const std::string &address, const unsigned short port);
public:
	HTTPListener(const std::string &address, const unsigned short port, OnMethod onpost, OnMethod onget);
	~HTTPListener();
	int start();
	void stop();
	void wait();
	OnMethod onPost;
	OnMethod onGet;
};

#endif