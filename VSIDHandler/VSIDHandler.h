#ifndef CAMPFIREBOT_VSIDHANDLER_H
#define CAMPFIREBOT_VSIDHANDLER_H

#include <curl/curl.h>
#include "HandlerAPI.h"
#include <vector>

class VSIDHandler
{
public:
	VSIDHandler(IHandler* pIHandler);
	~VSIDHandler();

   static std::vector<int> VSIDsFromMessage(const std::string& strMessage);
   static bool HasVSID(const std::string& strMessage);

   void PostResponseMessage(const std::string& strRoom, const std::string& strMessage);

protected:
	static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream);

protected:
   std::string GetResponseForVSID(int nID);
   bool GetInfoOutOfResponse(std::string& strTitle, std::string& strAssignedTo, std::string& strCurrentStatus, std::string& strSeverity, std::string& strPriority) const;

protected:
   IHandler* m_pIHandler;
	CURL *m_pCurl;
	CURLcode m_resLast;
	curl_slist * m_pCookies;

	std::string m_strWrite;
};


#endif
