#include "ScreencastLinkHandler.h"
#include "HandlerAPI.h"
#include <sstream>
#include <algorithm>//For std::transform
#include <stdlib.h>
#include <cstring>

#include <iostream>
using namespace std;

int StringToInt(const std::string& str)
{
   return atoi(str.c_str());
}

std::string IntToString(int nValue)
{
   std::ostringstream oss;
   oss << nValue;

   return oss.str();
}

std::string ToLower(const std::string& strCommand)
{
	std::string strReturn(strCommand);
	std::transform(strReturn.begin(), strReturn.end(), strReturn.begin(), ::tolower);
	return strReturn;
}

void Replace(std::string& s, const std::string& strToReplace, const std::string& strToReplaceWith)
{
   int nPos = 0;
   while(true)
   {
      nPos = s.find(strToReplace, nPos);
      if( nPos == std::string::npos )
         return;

      s.replace(nPos, strToReplace.length(), strToReplaceWith);
      nPos += strToReplaceWith.length();
   }
}

std::string UnEscapeXML(const std::string str)
{
   std::string strReturn(str);
   Replace(strReturn, "&quot;", "\"");
   Replace(strReturn, "&amp;", "&");
   Replace(strReturn, "&lt;", "<");
   Replace(strReturn, "&gt;", ">");
   Replace(strReturn, "&apos;", "\'");
   return strReturn;
}

bool HasItem(const std::vector<std::string>& arrstr, const std::string& str)
{
	for(std::vector<std::string>::size_type i=0; i<arrstr.size(); i++)
		if( arrstr[i] == str )
			return true;

	return false;
}

HANDLER_EXTERN int HandlerMessageSaid(void* pIFlowdockManager, const char* pstrOrg, const char* pstrFlow, int nType, int nUserID, const char* pstrMessage, int nMessageID)
{
   IHandler* pIHandler = (IHandler*)pIFlowdockManager;
   std::string strOrg(pstrOrg), strFlow(pstrFlow), strMessage(pstrMessage);

   if( ScreencastLinkHandler::HasSCLink(strMessage) )
   {
	   std::vector<std::string> arrstrLinks = ScreencastLinkHandler::SCLinksFromMessage(strMessage);
	   for(std::vector<std::string>::size_type i=0; i<arrstrLinks.size(); i++)
	   {
		   ScreencastLinkHandler sc(pIHandler);
		   std::string strImg = sc.GetImageURL(arrstrLinks[i]);
		   if( strImg.size() > 0 )
		   {
			   pIHandler->SayMessage(strOrg.c_str(), strFlow.c_str(), strImg.c_str(), nMessageID/*CommentTo*/, "");
		   }
	   }
   }

   return 0;
}

ScreencastLinkHandler::ScreencastLinkHandler(IHandler* pIHandler)
: m_pCurl(NULL), m_pCookies(NULL), m_pIHandler(pIHandler)
{
	//curl_global_init(CURL_GLOBAL_ALL);
	m_pCurl = curl_easy_init();

   curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, ScreencastLinkHandler::write_callback);
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this);

   curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

   curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(m_pCurl, CURLOPT_SSLENGINE_DEFAULT,1L);

	curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, 0L);
}

ScreencastLinkHandler::~ScreencastLinkHandler()
{
	curl_easy_cleanup(m_pCurl);
	//curl_global_cleanup();

	curl_slist_free_all(m_pCookies);
}

bool ScreencastLinkHandler::HasSCLink(const std::string& strMessage)
{
	return ScreencastLinkHandler::SCLinksFromMessage(strMessage).size() > 0;
}

std::vector<std::string> ScreencastLinkHandler::SCLinksFromMessage(const std::string& strMessage)
{
        cout << "Message is: " << strMessage << endl;
	std::vector<std::string> arrstrURLs;
	std::string strLower = ToLower( strMessage );

	int nStart = 0;
	while(true)
	{
		int nStartShortID = strLower.find("screencast.com/t/", nStart);
		if( nStartShortID == std::string::npos )
                {
                        cout << "not found screencast.com/t/ in: " << strLower << endl;
			break;
                }

		nStart = nStartShortID + strlen("screencast.com/t/");

		int nEnd = nStart;
		while(nEnd < (int)strLower.length() &&
         ((strLower[nEnd] >= 'a' && strLower[nEnd] <= 'z') || (strLower[nEnd] >= '0' && strLower[nEnd] <= '9')) )
			nEnd++;

		if( nEnd < (int)strLower.length() )
		{
			char ch = strLower[nEnd];
			if( ch != ' ' && ch != '<')
				continue;
		}


		if( nEnd > nStart )
		{
			std::string strShortID = strMessage.substr(nStart, nEnd-nStart);
			std::string strURL = "http://www.screencast.com/t/" + strShortID;
			if( !HasItem(arrstrURLs, strURL) )
				arrstrURLs.push_back(strURL);
		}
	}

	return arrstrURLs;
}

size_t ScreencastLinkHandler::write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	ScreencastLinkHandler* pHandler = (ScreencastLinkHandler*)stream;
	pHandler->m_strWrite.append((char*)ptr, nmemb);

	return nmemb;
}

std::string ScreencastLinkHandler::GetImageURL(const std::string& strURL)
{
   std::string strAddress = strURL;

   curl_easy_setopt(m_pCurl, CURLOPT_URL, strAddress.c_str());

   m_strWrite.clear();

   m_resLast = curl_easy_perform(m_pCurl);

   std::string strImageURL;
   int nStart = 0;
   while(true)
   {
	   int nStartImg = m_strWrite.find("<img", nStart);
	   if( nStartImg == std::string::npos )
		   break;

	   int nEndImg = m_strWrite.find(">", nStartImg + strlen("<img"));
	   if( nEndImg == std::string::npos )
		   break;

	   nStart = nStartImg + strlen("<img");

	   std::string strImgTag = m_strWrite.substr(nStartImg, nEndImg - nStartImg);
	   if( strImgTag.find("embeddedObject") == std::string::npos )
		   continue;

	   int nStartSrc = strImgTag.find("src=\"");
	   if( nStartSrc == std::string::npos )
		   continue;

	   int nEndSrc = strImgTag.find("\"", nStartSrc + strlen("src=\""));
	   if( nEndSrc == std::string::npos )
		   continue;

	   strImageURL = strImgTag.substr(nStartSrc + strlen("src=\""), nEndSrc - nStartSrc - strlen("src=\""));
	   Replace(strImageURL,  "%", "%25");

   }

   return strImageURL;
}

