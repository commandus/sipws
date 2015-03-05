#include "consolelocale.h"

#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <streambuf>


#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <codecvt>
#include <Windows.h>
#include <wincon.h>
#endif

/*
	localename	
		Windows "russian_russia.1251"
		Linux	"ru_RU.UTF-8"
*/
ConsoleLocale::ConsoleLocale(const std::string &localename, bool utf8)
{
#ifdef WIN32
	try 
	{
		std::locale(localename);
	}
	catch (std::exception) 
	{
	}
	if (utf8)
	{
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);
		// _setmode(_fileno(stdout), _O_U8TEXT);
		// _setmode(_fileno(stdin), _O_U8TEXT);

	}
#else
	try 
	{
		std::locale(localename);
	}
	catch (std::exception) {
	}
#endif
}


ConsoleLocale::~ConsoleLocale()
{
}
