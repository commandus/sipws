#include <iostream>
#include <stdlib.h>

typedef void(*onStopHandler)(int signal);

class SignalHandler
{
private:
	static void signal_SIGABRT_handler(int signum);
	static void signal_SIGFPE_handler(int signum);
	static void signal_SIGILL_handler(int signum);
	static void signal_SIGINT_handler(int signum);
	static void signal_SIGSEGV_handler(int signum);
	static void signal_SIGTERM_handler(int signum);
public:
	SignalHandler(std::ostream &cerr, onStopHandler onStop);
	~SignalHandler();
};

