#ifndef LIBRARY_H
#define LIBRARY_H

#include <string>

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class RLibrary
{
public:
	RLibrary(const std::string& strFilename);

	bool Load();
	void* Resolve(const char* pstrExport);
   std::string GetFullPath() const;

protected:
	std::string m_strFilename;

#ifdef WIN32
	HMODULE m_hModule;
#else
	void *m_hModule;
#endif
};

#endif

