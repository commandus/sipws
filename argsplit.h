#ifndef ARGSPLIT_H
#define ARGSPLIT_H

#include <string>

char **argsplit(const char *cmdline, int *argc);
char **argsplit(const std::string& cmdline, int *argc);
char **argappend(int *argc, int argc1, char **argv1, int argc2, char **argv2);

std::string extractPath(const char *modulename);
std::string loadCmdLine(const std::string path, const std::string &filename);

#endif