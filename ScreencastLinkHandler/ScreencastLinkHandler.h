#ifndef CAMPFIREBOT_VSIDHANDLER_H
#define CAMPFIREBOT_VSIDHANDLER_H

#include <curl/curl.h>
#include "HandlerAPI.h"
#include <vector>

class ScreencastLinkHandler
{
public:
	ScreencastLinkHandler(IHandler* pIHandler);
	~ScreencastLinkHandler();

   static bool HasSCLink(const std::string& strMessage);
   static std::vector<std::string> SCLinksFromMessage(const std::string& strMessage);

   std::string GetImageURL(const std::string& strURL);

protected:
	static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream);

protected:
   IHandler* m_pIHandler;
	CURL *m_pCurl;
	CURLcode m_resLast;
	curl_slist * m_pCookies;

	std::string m_strWrite;
};


#endif
