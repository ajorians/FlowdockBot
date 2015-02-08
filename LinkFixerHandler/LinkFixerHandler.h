#ifndef FlowdockBOT_LINKFIXERHANDLER_H
#define FlowdockBOT_LINKFIXERHANDLER_H

//#include <curl/curl.h>
#include "HandlerAPI.h"
#include <vector>
#include <string>

class LinkFixerHandler
{
public:
	LinkFixerHandler(IHandler* pIHandler);
	~LinkFixerHandler();

   static std::vector<std::string> LinksFromMessage(const std::string& strMessage);
   static bool HasLinks(const std::string& strMessage);
   static std::string GetCorrectedLink(const std::string& strLink);

   void PostResponseMessage(const std::string& strOrg, const std::string& strFlow, const std::string& strMessage, int nMessageID);

protected:
   IHandler* m_pIHandler;
};


#endif
