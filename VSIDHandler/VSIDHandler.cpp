#include "VSIDHandler.h"
#include "HandlerAPI.h"
#include <sstream>
#include <stdlib.h>
#include <cstring>

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

HANDLER_EXTERN int HandlerMessageSaid(void* pIFlowdockManager, const char* pstrOrg, const char* pstrFlow, int nType, int nUserID, const char* pstrMessage, int nMessageID)
{
   IHandler* pIHandler = (IHandler*)pIFlowdockManager;
   std::string strMessage(pstrMessage), strOrg(pstrOrg), strFlow(pstrFlow);

   if( VSIDHandler::HasVSID(strMessage) )
   {
      VSIDHandler vsid(pIHandler);
      vsid.PostResponseMessage(strOrg, strFlow, strMessage, nMessageID);
   }

   return 0;
}

VSIDHandler::VSIDHandler(IHandler* pIHandler)
: m_pCurl(NULL), m_pCookies(NULL), m_pIHandler(pIHandler)
{
	//curl_global_init(CURL_GLOBAL_ALL);
	m_pCurl = curl_easy_init();

   curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, VSIDHandler::write_callback);
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this);

   curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

   curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(m_pCurl, CURLOPT_SSLENGINE_DEFAULT,1L);

	curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, 0L);
}

VSIDHandler::~VSIDHandler()
{
	curl_easy_cleanup(m_pCurl);
	//curl_global_cleanup();

	curl_slist_free_all(m_pCookies);
}

std::vector<int> VSIDHandler::VSIDsFromMessage(const std::string& strMessage)
{
   std::vector<int> arrVSIDs;

   std::string strCurrentItem;
   for(std::string::size_type i=0; i<strMessage.length(); i++)
   {
      if( strMessage[i] >= '0' && strMessage[i] <= '9' )
      {
         strCurrentItem += strMessage[i];
      }
      else
      {
         if( strCurrentItem.length() > 0 && strCurrentItem.length() == 5 )
            arrVSIDs.push_back(StringToInt(strCurrentItem));
         strCurrentItem.clear();
      }
   }
   if( strCurrentItem.length() > 0 && strCurrentItem.length() == 5 )
      arrVSIDs.push_back(StringToInt(strCurrentItem));

   return arrVSIDs;
}

bool VSIDHandler::HasVSID(const std::string& strMessage)
{
   return VSIDsFromMessage(strMessage).size() > 0;
}

void VSIDHandler::PostResponseMessage(const std::string& strOrg, const std::string& strFlow, const std::string& strMessage, int nMessageID)
{
   std::vector<int> arrVSIDs = VSIDsFromMessage(strMessage);
   if( arrVSIDs.size() <= 0 )
      return;

   for(std::vector<int>::size_type i=0; i<arrVSIDs.size(); i++ )
   {
      std::string strResponse = GetResponseForVSID(arrVSIDs[i]);

      if( strResponse.length() > 0 )
         m_pIHandler->SayMessage(strOrg.c_str(), strFlow.c_str(), strResponse.c_str(), nMessageID/*CommentTo*/, "VSID");
   }
}

size_t VSIDHandler::write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	VSIDHandler* pHandler = (VSIDHandler*)stream;
	pHandler->m_strWrite.append((char*)ptr, nmemb);

	return nmemb;
}

std::string VSIDHandler::GetResponseForVSID(int nID)
{
   std::string strAddress = "http://tfs-app01:8080/tfs/web/UI/Pages/Reports/CustomReport.aspx?wid=";
   strAddress += IntToString(nID);

   curl_easy_setopt(m_pCurl, CURLOPT_URL, strAddress.c_str());

   curl_easy_setopt(m_pCurl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
   std::string strUserPasswd = "a.orians:!Q@W3e4r";
   curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPasswd.c_str());

   m_strWrite.clear();

   m_resLast = curl_easy_perform(m_pCurl);

   std::string strTitle, strAssignedTo, strCurrentStatus, strSeverity, strPriority;
   bool bOK = GetInfoOutOfResponse(strTitle, strAssignedTo, strCurrentStatus, strSeverity, strPriority);

   std::string strEditURL = "http://tfs-app01:8080/tfs/web/wi.aspx?id=";
   strEditURL += IntToString(nID);

   std::string strResponse = IntToString(nID);
   strResponse += ": ";
   if( bOK )
   {
      strResponse += strTitle + "(" + strCurrentStatus + ") - " + strAssignedTo + " - " + strSeverity + "/" + strPriority + " ";
      strResponse = UnEscapeXML(strResponse);
   }
   strResponse += strEditURL;

   return strResponse;
}

bool VSIDHandler::GetInfoOutOfResponse(std::string& strTitle, std::string& strAssignedTo, std::string& strCurrentStatus, std::string& strSeverity, std::string& strPriority) const
{
   {
      int nStartTitle = m_strWrite.find("<b>Title:</b></td><td>");
      int nEndTitle = m_strWrite.find("</td>", nStartTitle + strlen("<b>Title:</b></td><td>"));
      if(!( nStartTitle == std::string::npos || nEndTitle == std::string::npos || nStartTitle > nEndTitle ) )
         strTitle = m_strWrite.substr(nStartTitle + strlen("<b>Title:</b></td><td>"), nEndTitle - nStartTitle - strlen("<b>Title:</b></td><td>"));
   }

   {
      int nStartAssign = m_strWrite.find("<b>Assigned To:</b></td><td>");
      int nEndAssign = m_strWrite.find("</td>", nStartAssign + strlen("<b>Assigned To:</b></td><td>"));
      if( !( nStartAssign == std::string::npos || nEndAssign == std::string::npos || nStartAssign > nEndAssign ) )
         strAssignedTo = m_strWrite.substr(nStartAssign + strlen("<b>Assigned To:</b></td><td>"), nEndAssign - nStartAssign - strlen("<b>Assigned To:</b></td><td>"));
   }

   {
      int nStartStatus = m_strWrite.find("<b>Current Status:</b></td><td>");
      int nEndStatus = m_strWrite.find("</td>", nStartStatus + strlen("<b>Current Status:</b></td><td>"));
      if( !( nStartStatus == std::string::npos || nEndStatus == std::string::npos || nStartStatus > nEndStatus ) )
         strCurrentStatus = m_strWrite.substr(nStartStatus + strlen("<b>Current Status:</b></td><td>"), nEndStatus - nStartStatus - strlen("<b>Current Status:</b></td><td>"));
   }

   {
      int nStartSeverity = m_strWrite.find("<b>Severity:</b></td><td>");
      int nEndSeverity = m_strWrite.find("</td>", nStartSeverity + strlen("<b>Severity:</b></td><td>"));
      if( !( nStartSeverity == std::string::npos || nEndSeverity == std::string::npos || nStartSeverity > nEndSeverity ) )
         strSeverity = m_strWrite.substr(nStartSeverity + strlen("<b>Severity:</b></td><td>"), nEndSeverity - nStartSeverity - strlen("<b>Severity:</b></td><td>"));
   }

   {
      int nStartPriority = m_strWrite.find("<b>Priority:</b></td><td>");
      int nEndPriority = m_strWrite.find("</td>", nStartPriority + strlen("<b>Priority:</b></td><td>"));
      if( !( nStartPriority == std::string::npos || nEndPriority == std::string::npos || nStartPriority > nEndPriority ) )
         strPriority = m_strWrite.substr(nStartPriority + strlen("<b>Priority:</b></td><td>"), nEndPriority - nStartPriority - strlen("<b>Priority:</b></td><td>"));
   }

   return true;
}

