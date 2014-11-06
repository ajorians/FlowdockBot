#include "ConnectionManager.h"
#include "CampfireManager.h"
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
m_bExit(false), m_bDebugCampfireMessages(false)
{
#ifndef WIN32
   pthread_mutex_init(&m_mutex, NULL);
   pthread_mutex_init(&m_mutexResponses, NULL);
#endif
   int iRet;
   iRet = pthread_create( &m_thread, NULL, ConnectionManager::ConnectionManagerThread, (void*)this);

   m_pCampfireManager = new CampfireManager(this);
}

ConnectionManager::~ConnectionManager()
{
   Exit();
}

void ConnectionManager::SayMessage(const char* pstrRoom, const char* pstrMessage)
{
   std::string strRoom(pstrRoom), strMessage(pstrMessage);

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::SayMessage;
   msg.m_strRoom = strRoom;
   msg.m_strMessage = strMessage;

   m_arrQueuedMessages.push_back(msg);
}


void ConnectionManager::RestartCampfire(const char* pstrRoom)
{
   std::string strRoom(pstrRoom);

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::RestartCamp;
   msg.m_strRoom = strRoom;

   //IHandler methods assume m_mutex is already locked!
   m_arrQueuedMessages.push_back(msg);
}

void ConnectionManager::TrelloSubscribe(const char* pstrRoom, const char* pstrBoard, const char* pstrToken)
{
   std::string strRoom(pstrRoom), strBoard(pstrBoard), strToken(pstrToken);

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::TrelloSubscribe;
   msg.m_strRoom = strRoom;
   msg.m_strMessage = strBoard;
   msg.m_strToken = strToken;

   //IHandler methods assume m_mutex is already locked!
   m_arrQueuedMessages.push_back(msg);

   QueuedMessage msg2;
   msg2.m_eType = QueuedMessage::SayMessage;
   msg2.m_strMessage = "Subscribing to Trello board: " + strBoard;
   m_arrQueuedMessages.push_back(msg2);
}

void ConnectionManager::TrelloUnSubscribe(const char* pstrRoom, const char* pstrBoard)
{
   std::string strRoom(pstrRoom), strBoard(pstrBoard);

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::TrelloUnSubscribe;
   msg.m_strRoom = strRoom;
   msg.m_strMessage = strBoard;

   //IHandler methods assume m_mutex is already locked!
   m_arrQueuedMessages.push_back(msg);

   QueuedMessage msg2;
   msg2.m_eType = QueuedMessage::SayMessage;
   msg2.m_strRoom = strRoom;
   msg2.m_strMessage = "UnSubscribing from Trello board: " + strBoard;
   m_arrQueuedMessages.push_back(msg2);
}

void ConnectionManager::UploadMessage(const char* pstrRoom, const char* pstrFile, bool bDelete)
{
   std::string strRoom(pstrRoom), strFile(pstrFile);

   QueuedMessage msg;
   msg.m_eType = QueuedMessage::UploadFile;
   msg.m_strRoom = strRoom;
   msg.m_strMessage = strFile;
   msg.m_nAmount = bDelete ? 1 : 0;

   m_arrQueuedMessages.push_back(msg);
}

void ConnectionManager::MessageSaid(const std::string& strRoom, int nType, int nUserID, const std::string& strMessage)
{
   //called from a different thread
   pthread_mutex_lock( &m_mutex );

   //Notify chat handlers of this message
   NotifyHandlers(strRoom, nType, nUserID, strMessage);

   pthread_mutex_unlock( &m_mutex );

   if( m_bDebugCampfireMessages )
   {
      pthread_mutex_lock(&m_mutexResponses);
      std::string strCampfireMessage = "Camp: " + strMessage;
      m_arrResponses.push_back(strCampfireMessage);
      pthread_mutex_unlock(&m_mutexResponses);
   }
}

void ConnectionManager::TelloUpdate(const std::string& strRoom, const std::string& strName, const std::string& strListBefore, const std::string& strListAfter, const std::string& strDescription, bool bCreated, bool bClosed)
{
   //called from a different thread
   pthread_mutex_lock( &m_mutex );

   NotifyHandlers(strRoom, strName, strListBefore, strListAfter, strDescription, bCreated, bClosed);

   pthread_mutex_unlock( &m_mutex );
}

void ConnectionManager::Exit()
{
   pthread_mutex_lock( &m_mutex );

   if( m_pCampfireManager )
   {
      m_pCampfireManager->Exit();
      delete m_pCampfireManager;
      m_pCampfireManager= NULL;
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
   m_bDebugCampfireMessages = !m_bDebugCampfireMessages;

   pthread_mutex_lock( &m_mutexResponses );
   std::string strMsg = "Debug is ";
   if( m_bDebugCampfireMessages )
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

bool ConnectionManager::JoinRoom(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL)
{
   bool bOK = false;

   bOK = Rejoin(strCamp, strAuth, strRoom, bSSL);

   return bOK;
}

bool ConnectionManager::LeaveRoom(const std::string& strRoom)
{
   bool bOK = false;

   bOK = Leave(strRoom);

   return bOK;
}

bool ConnectionManager::Say(const std::string& strRoom, const std::string& strMessage)
{
   pthread_mutex_lock( &m_mutex );

   SayMessage(strRoom.c_str(), strMessage.c_str());
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

bool ConnectionManager::StartTrelloUpdate(const std::string& strRoom)
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::RestartTrello;
   msg.m_strRoom = strRoom;
   msg.strRoom = "501ffde81e3dd9d81413ea3d";//"504f354d4289bab413347dac";
   msg.m_strToken = "4d9e17de6229f0ff8e225eba644e5fb240f2bb4e867967ce7443b629c560de42";

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

      std::vector<std::string> arrRooms = m_pCampfireManager->GetRoomList();

      pthread_mutex_lock( &m_mutex );
      DoTimedEvents(arrRooms);
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

      if( msg.m_eType == QueuedMessage::RestartCamp )
      {
         m_pCampfireManager->JoinRoom(msg.strCamp, msg.strUsername, msg.strRoom, msg.bSSL);
      }
      else if( msg.m_eType == QueuedMessage::LeaveRoom )
      {
         m_pCampfireManager->LeaveRoom(msg.strRoom);
      }
      else if( msg.m_eType == QueuedMessage::SayMessage )
      {
         m_pCampfireManager->Say(msg.m_strRoom, msg.m_strMessage);
      }
      else if( msg.m_eType == QueuedMessage::UploadFile )
      {
         m_pCampfireManager->Upload(msg.m_strRoom, msg.m_strMessage, msg.m_nAmount == 1 ? true : false);
      }
      else if( msg.m_eType == QueuedMessage::AdjustUpdateFrequency )
      {
         m_pCampfireManager->ChangeUpdateFrequency(msg.m_nAmount);
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

void ConnectionManager::DoTimedEvents(const std::vector<std::string>& arrRooms)
{
   for(std::vector<RLibrary*>::size_type i = 0; i<m_arrChatHandlers.size(); i++)
   {
      HandlerTimeEventFunc TimeEvent = (HandlerTimeEventFunc)m_arrChatHandlers[i]->Resolve("HandlerTimeEvent");
      if( !TimeEvent )
         continue;

      for(std::vector<std::string>::size_type j=0; j<arrRooms.size(); j++)
      {
         TimeEvent(this, arrRooms[j].c_str());
      }
   }
}

bool ConnectionManager::Rejoin(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL)
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::RestartCamp;
   msg.strCamp = strCamp;
   msg.strUsername = strAuth;
   msg.strRoom = strRoom;
   msg.bSSL = bSSL;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool ConnectionManager::Leave(const std::string& strRoom)
{
   QueuedMessage msg;
   msg.m_eType = QueuedMessage::LeaveRoom;
   msg.strRoom = strRoom;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

void ConnectionManager::NotifyHandlers(const std::string& strRoom, int nType, int nUserID, const std::string& strMessage)
{
   for(std::vector<RLibrary*>::size_type i = 0; i<m_arrChatHandlers.size(); i++)
   {
      HandlerMessageSaidFunc Said = (HandlerMessageSaidFunc)m_arrChatHandlers[i]->Resolve("HandlerMessageSaid");
      if( !Said )
         continue;

      Said(this, strRoom.c_str(), nType, nUserID, strMessage.c_str());
   }
}

void ConnectionManager::NotifyHandlers(const std::string& strRoom, const std::string& strName, const std::string& strListBefore, const std::string& strListAfter, const std::string& strDescription, bool bCreated, bool bClosed)
{
   for(std::vector<RLibrary*>::size_type i = 0; i<m_arrChatHandlers.size(); i++)
   {
      HandlerTrelloAdjustmentFunc Adjust = (HandlerTrelloAdjustmentFunc)m_arrChatHandlers[i]->Resolve("HandlerTrelloAdjustment");
      if( !Adjust )
         continue;

      Adjust(this, strRoom.c_str(), strName.c_str(), strListBefore.c_str(), strListAfter.c_str(), strDescription.c_str(), bCreated ? 1 : 0, bClosed ? 1 : 0);
   }
}




