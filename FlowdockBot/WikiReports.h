#include <string>
#include <vector>
#include <curl/curl.h>

class WikiReports
{
public:
	WikiReports();
	~WikiReports();

   bool LogonAndEdit(std::string strUsername, std::string strPassword, std::string strPageToEdit, std::string strPageToEditHTML, std::string strTitle);

   bool GetPageBody(std::string strPageToEdit, std::string& strPageBody);

protected:
	static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream);

protected:
	CURL* m_pCurl;
	CURLcode m_resLast;
   curl_slist * m_pCookies;
	std::string m_strWrite;
};


