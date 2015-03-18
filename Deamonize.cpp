#include "Deamonize.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#pragma comment(lib, "advapi32.lib")

SERVICE_STATUS        g_ServiceStatus = {0}; 
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;

#else

#include <unistd.h>
#include <syslog.h>

#endif

static std::string serviceName;
static TDeamonRunner daemonRun;
static TDeamonRunner daemonStopRequest;
static TDeamonRunner daemonDone;


Deamonize::Deamonize(const std::string &daemonName, 
	TDeamonRunner runner, TDeamonRunner stopRequest, TDeamonRunner done)
	
{
	serviceName = daemonName;
	daemonRun = runner;
	daemonStopRequest = stopRequest;
	daemonDone = done;
	int r = init();
	if (r)
	{
		std::cerr << "Error daemonize " << r << std::endl;
	}
}

Deamonize::~Deamonize()
{
}

#ifdef WIN32

void statusStartPending()
{
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T(
			"ServiceMain: SetServiceStatus returned error"));
	}
}

void statusStarted()
{
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T(
			"ServiceMain: SetServiceStatus returned error"));
	}
}

void statusStopPending()
{
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 4;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T(
			"ServiceCtrlHandler: SetServiceStatus returned error"));
	}
}

void statusStopped()
{
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;
	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T(
			"ServiceMain: SetServiceStatus returned error"));
	}
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;
		statusStopPending();
		daemonStopRequest();
		daemonDone();
		statusStopped();
		break;
	default:
		break;
	}
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	// Register our service control handler with the SCM
	g_StatusHandle = RegisterServiceCtrlHandler((LPWSTR) serviceName.c_str(), ServiceCtrlHandler);
	if (g_StatusHandle == NULL)
		goto EXIT;
	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	statusStartPending();
	// Tell the service controller we are started
	statusStarted();
	daemonRun();
	daemonDone();
	// Tell the service controller we are stopped
	statusStopped();
EXIT:
	return;
}

// See http://stackoverflow.com/questions/18557325/how-to-create-windows-service-in-c-c
int Deamonize::init()
{
	std::wstring sn(serviceName.begin(), serviceName.end());
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{(LPWSTR)sn.c_str(), (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};
	if (StartServiceCtrlDispatcher (ServiceTable) == FALSE)
		return GetLastError ();
	return 0;
}

#else
//See http://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
int Deamonize::init()
{
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Catch, ignore and handle signals */
	//TODO: Implement a working signal handler */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
	{
		close(x);
	}
	
	daemonRun();
	daemonDone();
	return 0;
}

#endif
