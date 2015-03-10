#include "LoggerFunc.h"
#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif


TLoggerFunc loadLogger(const std::string &name, const std::string &funcname, void **handle)
{
	TLoggerFunc r = NULL;
	std::string p(name);
#ifdef WIN32
	p = ".\\" + p + ".dll";
	*handle = LoadLibraryA(p.c_str());
	if (*handle)
		r = (TLoggerFunc)GetProcAddress((HMODULE) *handle, funcname.c_str());
#else
	p = "./" + p + ".so";
	*handle = dlopen(p.c_str(), RTLD_NOW);
	if (*handle)
		r = (TLoggerFunc) dlsym(*handle, funcname.c_str());

#endif
	return r;
}

void closeLibrary(void *handle)
{
	if (handle == NULL)
		return;
#ifdef WIN32
	FreeLibrary((HMODULE) handle);
#else
	dlclose(handle);
#endif
}

