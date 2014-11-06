#include "Paths.h"

#ifdef WIN32
#include <Windows.h>
#include <atlstr.h>
#endif

#ifdef __linux__ 
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#endif

#ifndef NULL
#define NULL   0
#endif

#ifdef WIN32
std::string ws2s(const std::wstring& s)
{
   int len;
   int slength = (int)s.length() + 1;
   len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0); 
   char* buf = new char[len];
   WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0); 
   std::string r(buf);
   delete[] buf;
   return r;
}
#endif

std::string GetSelfPath()
{
#ifdef WIN32
   TCHAR szPath[MAX_PATH];
   GetModuleFileName(NULL, (TCHAR*)&szPath, MAX_PATH);
   std::wstring strwPath((wchar_t*)(TCHAR*)&szPath);
   return ws2s(strwPath);
#else
   char result[ PATH_MAX ];
   ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
   return std::string( result, (count > 0) ? count : 0 );
#endif
}

std::string GetSelfDirectory()
{
   std::string strPath = GetSelfPath();
#ifdef WIN32
   std::string strPathDirectory = strPath.substr(0, strPath.find_last_of('\\'));
   strPathDirectory += "\\";
#else
   std::string strPathDirectory = strPath.substr(0, strPath.find_last_of('/'));
   strPathDirectory += "/";
#endif
   return strPathDirectory;
}

