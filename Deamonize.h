#ifndef DEAMONIZE_H
#define DEAMONIZE_H	1

#include <string>

typedef void(*TDeamonRunner)();

// See http://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
class Deamonize
{
private:
	int init();
public:
	Deamonize(const std::string &daemonName, 
		TDeamonRunner runner, TDeamonRunner stopRequest, TDeamonRunner done);
	~Deamonize();
};

#endif

