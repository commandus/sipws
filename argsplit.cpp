#include "argsplit.h"
#include "sipwsUtil.h"
#include <algorithm>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <wordexp.h>
#endif

void argfree(int *argc, char ** argv)
{
	if (argv)
	{
		for (int i = 0; i < *argc; i++)
		{
			if (argv[i])
				free(argv[i]);
		}
		free(argv);
	}
	*argc = 0;
}

char **argsplit(const std::string& cmdline, int *argc)
{
	return argsplit(cmdline.c_str(), argc);
}

// See http://stackoverflow.com/questions/1706551/parse-string-into-argv-argc/10071763#10071763
char **argsplit(const char *cmdline, int *argc)
{
	char **argv = NULL;
	if ((!cmdline) || (!argc))
		return NULL;
	int i;
#ifndef _WIN32
	{
		wordexp_t p;
		// Note! This expands shell variables.
		if (wordexp(cmdline, &p, 0))
			return NULL;
		*argc = p.we_wordc;
		if (!(argv = (char**) calloc(*argc, sizeof(char *))))
			goto fail;

		for (i = 0; i < p.we_wordc; i++)
		{
			if (!(argv[i] = strdup(p.we_wordv[i])))
				goto fail;
		}
		wordfree(&p);
		return argv;
	fail:
		wordfree(&p);
	}
#else // WIN32
	{
		wchar_t **wargs = NULL;
		size_t needed = 0;
		wchar_t *cmdlinew = NULL;
		size_t len = strlen(cmdline) + 1;

		if (!(cmdlinew = (wchar_t *)calloc(len, sizeof(wchar_t))))
			goto fail;
		if (strlen(cmdline) == 0)
		{
			*argc = 0;
		}
		else
		{
			if (!MultiByteToWideChar(CP_ACP, 0, cmdline, -1, cmdlinew, len))
				goto fail;
			if (!(wargs = CommandLineToArgvW(cmdlinew, argc)))
				goto fail;
			if (!(argv = (char**)calloc(*argc, sizeof(char *))))
				goto fail;
		}

		// Convert from wchar_t * to ANSI char *
		for (i = 0; i < *argc; i++)
		{
			// Get the size needed for the target buffer.
			// CP_ACP = Ansi Codepage.
			needed = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1,
				NULL, 0, NULL, NULL);

			if (!(argv[i] = (char *) malloc(needed)))
				goto fail;

			// Do the conversion.
			needed = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1,
				argv[i], needed, NULL, NULL);
		}

		if (wargs) LocalFree(wargs);
		if (cmdlinew) free(cmdlinew);
		return argv;

	fail:
		if (wargs) LocalFree(wargs);
		if (cmdlinew) free(cmdlinew);
	}
#endif // WIN32
	argfree(argc, argv);
	return NULL;
}

char **argappend(int *argcres, int argc1, char **argv1, int argc2, char **argv2)
{
	int len = argc1 + argc2;
	char** r = (char**)calloc(len, sizeof(char *));
	char *p;
	for (int i = 0; i < len; i++)
	{
		if (i < argc1)
			p = argv1[i];
		else
			p = argv2[i - argc1];
		size_t l = strlen(p);
		if ((r[i] = (char *)malloc(l + 1)))
			memcpy(r[i], p, l + 1);
	}
	*argcres = len;
	return  r;
}

std::string extractPath(const char *modulename)
{
	struct MatchPathSeparator
	{
		bool operator()(char ch) const
		{
			return ch == '\\' || ch == '/';
		}
	};

	// char modulename[MAX_PATH];	GetModuleFileNameA(NULL, modulename, MAX_PATH - 1);
	std::string r(modulename);
	r = std::string(r.begin(), std::find_if(r.rbegin(), r.rend(), MatchPathSeparator()).base());
	return r;
}

std::string loadCmdLine(const std::string binpath, const std::string &filename)
{
	// check program directory
	std::string r;
	r = readFile(binpath + filename);
	if (!r.empty())
		return r;

	// check other directories
#ifdef WIN32
	const std::string path[]
	{
		".\\", "\\Windows\\"
	};
#else
	const std::string path[]
	{
		"./", "/etc/"
	};
#endif
	for (int i = 0; i < 2; i++)
	{
		r = readFile(path[i] + filename);
		if (!r.empty())
			return r;
	}
	return "";
}
