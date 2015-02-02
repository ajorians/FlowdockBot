#include "ConnectionManager.h"
#include "FlowdockManager.h"
#include "Paths.h"

#include <iostream>

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

ConnectionManager::ConnectionManager()
: 
#ifdef WIN32
m_mutex(PTHREAD_MUTEX_INITIALIZER),
m_mutexResponses(PTHREAD_MUTEX_INITIALIZER),
#endif
m_bExit(false), m_bDebugFlowdockMessages(false)
{
#ifndef WIN32
   pthread_mutex_init(&m_mutex, NULL);
   pthread_mutex_init(&m_mutexResponses, NULL);
#endif
   int iRet;
   iRet = pthread_create( &m_thread, NULL, ConnectionManager::ConnectionManagerThread, (void*)this);

   m_pFlowdockManager = new FlowdockManager(this);
}

ConnectionManager::~ConnectionManager()
{
   Exit();
}

void ConnectionManager::SayMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrMessage, int nCommentTo, const char* pstrTags)
{
   std::string strOrg(pstrOrg), strRoom(pstrRoom), strMessage(pstrMessage), strTags(pstrTags), strExternalUser("build_bot");

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::SayMessage;
   msg.m_strOrg = strOrg;
   msg.m_strFlow = strRoom;
   msg.m_strMessage = strMessage;
   msg.m_nCommentTo = nCommentTo;
   msg.m_strTags = strTags;
   msg.m_strExternalUser = strExternalUser;

   m_arrQueuedMessages.push_back(msg);
}

void ConnectionManager::UploadMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrFile)
{
   std::string strOrg(pstrOrg), strRoom(pstrRoom), strFile(pstrFile);

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::UploadFile;
   msg.m_strOrg = strOrg;
   msg.m_strFlow = strRoom;
   msg.m_strMessage = strFile;
   msg.m_nCommentTo = -1;//TODO: Perhaps allow comment to.

   m_arrQueuedMessages.push_back(msg);
}

void ConnectionManager::MessageSaid(const std::string& strOrg, const std::string& strFlow, int nType, int nUserID, const std::string& strMessage)
{
   //called from a different thread
   pthread_mutex_lock( &m_mutex );

   //Notify chat handlers of this message
   NotifyHandlers(strOrg, strFlow, nType, nUserID, strMessage);

   pthread_mutex_unlock( &m_mutex );

   if( m_bDebugFlowdockMessages )
   {
      pthread_mutex_lock(&m_mutexResponses);
      std::string strFlowdockMessage = "Camp: " + strMessage;
      m_arrResponses.push_back(strFlowdockMessage);
      pthread_mutex_unlock(&m_mutexResponses);
   }
}

void ConnectionManager::Exit()
{
   pthread_mutex_lock( &m_mutex );

   if( m_pFlowdockManager )
   {
      m_pFlowdockManager->Exit();
      delete m_pFlowdockManager;
      m_pFlowdockManager= NULL;
   }

   m_bExit = true;
   pthread_mutex_unlock( &m_mutex );

   pthread_join( m_thread, NULL);
}

bool ConnectionManager::ReloadChatHandlers()
{
   bool bRet = LoadChatHandlers();

   return bRet;
}

bool ConnectionManager::ListChatHandlers()
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::ListHandlers;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

void ConnectionManager::ToggleDebugMessages()
{
   m_bDebugFlowdockMessages = !m_bDebugFlowdockMessages;

   pthread_mutex_lock( &m_mutexResponses );
   std::string strMsg = "Debug is ";
   if( m_bDebugFlowdockMessages )
   {
      strMsg += "ON";
   }
   else
   {
      strMsg += "OFF";
   }
   m_arrResponses.push_back(strMsg);
   pthread_mutex_unlock( &m_mutexResponses );
}

bool ConnectionManager::JoinRoom(const std::string& strOrg, const std::string& strFlow)
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::JoinRoom;
   msg.m_strOrg = strOrg;
   msg.m_strFlow = strFlow;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool ConnectionManager::Connect(const std::string& strUsername, const std::string& strPassword)
{
   bool bOK = false;

   bOK = Rejoin(strUsername, strPassword);

   return bOK;
}

bool ConnectionManager::Say(const std::string& strOrg, const std::string& strRoom, const std::string& strMessage, int nCommentTo)
{
   pthread_mutex_lock( &m_mutex );

   SayMessage(strOrg.c_str(), strRoom.c_str(), strMessage.c_str(), nCommentTo, "");
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool ConnectionManager::ChangeUpdateFrequency(int nMS)
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::AdjustUpdateFrequency;
   msg.m_nAmount = nMS;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool ConnectionManager::GetResponce(std::string& strResponse)
{
   bool bOK = false;

   pthread_mutex_lock( &m_mutexResponses );
   if( m_arrResponses.size() > 0 )
   {
      strResponse = m_arrResponses[0];
      m_arrResponses.erase(m_arrResponses.begin());
      bOK = true;
   }
   pthread_mutex_unlock( &m_mutexResponses );

   return bOK;
}

void* ConnectionManager::ConnectionManagerThread(void* ptr)
{
   ConnectionManager* pThis = (ConnectionManager*)ptr;
   pThis->DoWork();

   return NULL;
}

void ConnectionManager::DoWork()
{
   while(true)
   {
#ifdef WIN32
      Sleep(10);
#else
      usleep(10 * 1000);
#endif
      bool bExit = false;
      pthread_mutex_lock( &m_mutex );
      if( m_bExit )
         bExit = true;
      pthread_mutex_unlock( &m_mutex );

      if( bExit )
         break;

      pthread_mutex_lock( &m_mutex );
      DoQueuedMessages();
      pthread_mutex_unlock( &m_mutex );

      pthread_mutex_lock( &m_mutex );
      DoQueuedMessages();
      pthread_mutex_unlock( &m_mutex );
   }
}

bool ConnectionManager::LoadChatHandlers()
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::ReloadHandlers;

   pthread_mutex_lock( &m_mutex );
   //std::cout << "Reloading chat handlers" << std::endl;
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

void ConnectionManager::DoQueuedMessages()
{
   while( m_arrQueuedMessages.size() > 0 )
   {
      QueuedMessage msg = m_arrQueuedMessages[0];

      if( msg.m_eType == QueuedMessage::JoinRoom )
      {
         m_pFlowdockManager->AddFlow(msg.m_strOrg, msg.m_strFlow);
      }
      else if( msg.m_eType == QueuedMessage::RestartCamp )
      {
         m_pFlowdockManager->Connect(msg.strUsername, msg.strPassword);
      }
      else if( msg.m_eType == QueuedMessage::SayMessage )
      {
         m_pFlowdockManager->Say(msg.m_strOrg, msg.m_strFlow, msg.m_strMessage, msg.m_nCommentTo, msg.m_strTags, msg.m_strExternalUser);
      }
      else if( msg.m_eType == QueuedMessage::UploadFile )
      {
         m_pFlowdockManager->Upload(msg.m_strFlow, msg.m_strMessage);
      }
      else if( msg.m_eType == QueuedMessage::AdjustUpdateFrequency )
      {
         m_pFlowdockManager->ChangeUpdateFrequency(msg.m_nAmount);
      }
      else if( msg.m_eType == QueuedMessage::ListHandlers )
      {
         std::string strMessage = "Handlers:\n";
         for(std::vector<RLibrary*>::size_type i=0; i<m_arrChatHandlers.size(); i++)
         {
            strMessage += m_arrChatHandlers[i]->GetFullPath();
            strMessage += "\n";
         }

         pthread_mutex_lock( &m_mutexResponses );
         m_arrResponses.push_back(strMessage);
         pthread_mutex_unlock( &m_mutexResponses );
      }
      else if( msg.m_eType == QueuedMessage::ReloadHandlers )
      {
         //Free any existing chat handlers

         std::string strPath = GetSelfDirectory() + "Handlers";
#ifdef WIN32
         strPath += "\\";
#else
         strPath += "/";
#endif
         std::string strSearchPath = strPath + "*.*";

         //std::cout << "strSearchPath: " << strSearchPath << std::endl;
#ifdef WIN32
         WIN32_FIND_DATA ffd;
         HANDLE hFind = INVALID_HANDLE_VALUE;

         hFind = FindFirstFile(CString(strSearchPath.c_str()), &ffd);
         do
         {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
            }
            else
            {
               CT2CA pszConvertedAnsiString (ffd.cFileName);
               std::string strFile (pszConvertedAnsiString);
#else
         DIR *dp;
         struct dirent *dirp;
         if((dp  = opendir(strPath.c_str())) == NULL) {
            //std::cout << "Error(" << errno << ") opening " << std::dir << endl;
            //return errno;
         }

         while ((dirp = readdir(dp)) != NULL) {

            std::string strFile(std::string(dirp->d_name));
#endif      
            if( strFile.find(MODULE_EXT) != std::string::npos )
            {
               std::string strDLL = strPath + strFile;
               RLibrary* pLibHandler = new RLibrary(strDLL);
               pLibHandler->Load();
               m_arrChatHandlers.push_back(pLibHandler);
            }
#ifdef WIN32
         }
      }
      while (FindNextFile(hFind, &ffd) != 0);
#else
         }
         closedir(dp);
#endif

      }
      else
      {

      }

      m_arrQueuedMessages.erase(m_arrQueuedMessages.begin());
   }
}

bool ConnectionManager::Rejoin(const std::string& strUsername, const std::string& strPassword)
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::RestartCamp;
   msg.strUsername = strUsername;
   msg.strPassword = strPassword;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

void ConnectionManager::NotifyHandlers(const std::string& strOrg, const std::string& strFlow, int nType, int nUserID, const std::string& strMessage)
{
   for(std::vector<RLibrary*>::size_type i = 0; i<m_arrChatHandlers.size(); i++)
   {
      HandlerMessageSaidFunc Said = (HandlerMessageSaidFunc)m_arrChatHandlers[i]->Resolve("HandlerMessageSaid");
      if( !Said )
         continue;

      Said(this, strOrg.c_str(), strFlow.c_str(), nType, nUserID, strMessage.c_str());
   }
}


