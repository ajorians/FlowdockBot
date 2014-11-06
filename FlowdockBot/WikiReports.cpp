#include "WikiReports.h"
#include "stdio.h"
#include <string.h>
#include <iostream>

using namespace std;

WikiReports::WikiReports()
{
	curl_global_init(CURL_GLOBAL_ALL);
	m_pCurl = curl_easy_init();

	//curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, 1L);
}

WikiReports::~WikiReports()
{
	curl_easy_cleanup(m_pCurl);
	curl_global_cleanup();
}

size_t WikiReports::write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	WikiReports* pGraph = (WikiReports*)stream;
	pGraph->m_strWrite.append((char*)ptr, nmemb);

	return nmemb;
}

string EscapeURL(string strInput)
{
   //Turns: Products/Camtasia_Studio/Camtasia_Studio_7.0/Team_Documentation/Daily_Scrum_Reporting_Notes/somepage
   //Into:  Products/Camtasia+Studio/Camtasia+Studio+7.0/Team+Documentation/Daily+Scrum+Reporting+Notes
   string strReturn = strInput;
   while( true )
   {
      int nPos = strReturn.find("_");
      if( nPos == string::npos )
         break;
      strReturn.replace(nPos, 1, "+");
   }
   //strReturn.replace(0, "_", "+");
   return strReturn;
}

bool WikiReports::GetPageBody(std::string strPageToEdit, std::string& strPageBody)
{
   CURLcode m_resLast;
   std::string strURL;

	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, WikiReports::write_callback);
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this);

   curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

   strURL = "http://wiki.techsmith.com/";
   strURL += strPageToEdit;

   curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
  
   struct curl_slist *pHeader = curl_slist_append(NULL, "Pragma: no-cache");

	curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, pHeader);

   m_resLast = curl_easy_perform(m_pCurl);

   curl_slist_free_all(pHeader);

   if( m_resLast != CURLE_OK )
		return false;

   int nStart = m_strWrite.find("<div class=\"pageText\" id=\"pageText\">");
   if( nStart == string::npos )
      return false;

   int nEnd = m_strWrite.find("</div>", nStart);
   if( nEnd == string::npos )
      return false;

   if( nStart >= nEnd )
      return false;

   string str = m_strWrite.substr(nStart + strlen("<div class=\"pageText\" id=\"pageText\">"), nEnd - nStart - strlen("<div class=\"pageText\" id=\"pageText\">"));

   strPageBody = str;

   return true;
}

bool WikiReports::LogonAndEdit(string strUsername, string strPassword, string strPageToEdit, string strPageToEditHTML, string strTitle)
{
   CURLcode m_resLast;
   std::string strURL;

	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, WikiReports::write_callback);
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this);

   curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

   struct curl_httppost *formpost=NULL;
   struct curl_httppost *lastptr=NULL;

   strURL = "http://wiki.techsmith.com/index.php?title=Special:Userlogin&returntotitle=";
   strURL += strPageToEdit;

   curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());

   curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "auth_id",
                 CURLFORM_COPYCONTENTS, "12",
                 CURLFORM_END);

   curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "username",
                 CURLFORM_COPYCONTENTS, strUsername.c_str(),
                 CURLFORM_END);

   string strURLEscaped = EscapeURL(strPageToEdit);
   curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "returntotitle",
                 CURLFORM_COPYCONTENTS, strURLEscaped.c_str(),
                 CURLFORM_END);

   curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "password",
                 CURLFORM_COPYCONTENTS, strPassword.c_str(),
                 CURLFORM_END);

   curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "deki_buttons",
                 CURLFORM_COPYCONTENTS, "action",
                 CURLFORM_END);

   curl_easy_setopt(m_pCurl, CURLOPT_HTTPPOST, formpost);

   curl_easy_setopt(m_pCurl, CURLOPT_COOKIEFILE, "");

	m_resLast = curl_easy_perform(m_pCurl);

   curl_easy_getinfo(m_pCurl, CURLINFO_COOKIELIST, &m_pCookies);

   curl_formfree(formpost);

	if( m_resLast != CURLE_OK )
		return false;

   m_strWrite.clear();

	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, WikiReports::write_callback);
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this);

   curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

   strURL = "http://wiki.techsmith.com/index.php?title=";
   strURL += strPageToEdit;
   strURL += "&action=submit&text=Products/Camtasia%20Studio/Camtasia%20Studio%207.0/Team%20Documentation/Daily%20Scrum";

   curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());

   formpost=NULL;
   lastptr=NULL;

   curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "wpTextbox1",
                 CURLFORM_COPYCONTENTS, strPageToEditHTML.c_str(),
                 CURLFORM_END);

   curl_formadd(&formpost,
              &lastptr,
              CURLFORM_COPYNAME, "wpPath",
              CURLFORM_COPYCONTENTS, strPageToEdit.c_str(),
              CURLFORM_END);

   curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "wpSummary",
        CURLFORM_COPYCONTENTS, "",
        CURLFORM_END);

   curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "displaytitle",
        CURLFORM_COPYCONTENTS, strTitle.c_str(),
        CURLFORM_END);

   curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "wpSection",
        CURLFORM_COPYCONTENTS, "",
        CURLFORM_END);

   time_t now;
	now = time(NULL);
   char buffer[255];
   strftime(buffer, 255, "%Y", gmtime(&now));
   string strYear = buffer;
   strftime(buffer, 255, "%m", gmtime(&now));
   string strMonth = buffer;
   strftime(buffer, 255, "%d", gmtime(&now));
   string strDay = buffer;
   strftime(buffer, 255, "%H", gmtime(&now));
   string strHour = buffer;
   strftime(buffer, 255, "%M", gmtime(&now));
   string strMinute = buffer;
   strftime(buffer, 255, "%S", gmtime(&now));
   string strSecond = buffer;

   string strTime = strYear;
   strTime += strMonth;
   strTime += strDay;
   strTime += strHour;
   strTime += strMinute;
   strTime += strSecond;

   curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "wpEdittime",
        CURLFORM_COPYCONTENTS, strTime.c_str(),//"20090917160900",
        CURLFORM_END);

   curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "wpArticleId",
        CURLFORM_COPYCONTENTS, "7251",
        CURLFORM_END);

   curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, "wpEditToken",
        CURLFORM_COPYCONTENTS, "2a906be7d8226842748cecd8e6eab71c",
        CURLFORM_END);

   curl_easy_setopt(m_pCurl, CURLOPT_HTTPPOST, formpost);

   curl_easy_setopt(m_pCurl, CURLOPT_COOKIEFILE, &m_pCookies);

	m_resLast = curl_easy_perform(m_pCurl);

	if( m_resLast != CURLE_OK )
		return false;

   return true;//Could use more error checking.
}

