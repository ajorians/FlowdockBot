#ifndef PATHS_H
#define PATHS_H

#include <string>

#ifdef WIN32
#define MODULE_EXT	".dll"
#else
#define MODULE_EXT	".so"
#endif

#ifdef WIN32
std::string ws2s(const std::wstring& s);
#endif

std::string GetSelfPath();
std::string GetSelfDirectory();

#endif
