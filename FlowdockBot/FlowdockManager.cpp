#include "FlowdockManager.h"
#include "ConnectionManager.h"
#include <iostream>
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
#define Flowdock_MODULE "FlowdockAPI.dll"
#else
#define Flowdock_MODULE "libFlowdockAPI.so"
#endif

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

std::string URLEncode(const std::string str)
{
   std::string strReturn(str);
   Replace(strReturn, ";", "%3B");
   return strReturn;
}

FlowdockManager::FlowdockManager(IManager* pManager)
: 
#ifdef WIN32
m_mutex(PTHREAD_MUTEX_INITIALIZER),
#endif
 m_libraryFlowdock(GetSelfDirectory() + Flowdock_MODULE
), m_pManager(pManager), m_bExit(false), m_nCurrentWaitTime(5000), m_FlowdockInstance(NULL)
{
#ifndef WIN32
   pthread_mutex_init(&m_mutex, NULL);
#endif
   if( !m_libraryFlowdock.Load() )
   {
      std::cout << "Couldn't load FlowdockAPI" << std::endl;
      return;
   }
   int iRet;
   iRet = pthread_create( &m_thread, NULL, FlowdockManager::FlowdockManagerThread, (void*)this);
}

FlowdockManager::~FlowdockManager()
{
   Exit();
}

void FlowdockManager::Exit()
{
   pthread_mutex_lock( &m_mutex );
   m_bExit = true;
   pthread_mutex_unlock( &m_mutex );

   pthread_join( m_thread, NULL);

   FlowdockFreeFunc FreeAPI = (FlowdockFreeFunc)m_libraryFlowdock.Resolve("FlowdockFree");
   if( FreeAPI )
   {
      FreeAPI(&m_FlowdockInstance);
      m_FlowdockInstance = NULL;
   }
}

bool FlowdockManager::Say(const std::string& strOrg, const std::string& strFlow, const std::string& strMessage)
{
   FlowdockQueuedMessage msg;
   msg.m_eType = FlowdockQueuedMessage::SayMessage;
   msg.m_strOrg = strOrg;
   msg.m_strFlow = strFlow;
   msg.m_strMessage = strMessage;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool FlowdockManager::Upload(const std::string& strRoom, const std::string& strFile)
{
   FlowdockQueuedMessage msg;
   msg.m_eType = FlowdockQueuedMessage::UploadFile;
   msg.m_strFlow = strRoom;
   msg.m_strMessage = strFile;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool FlowdockManager::ChangeUpdateFrequency(int nMS)
{
   pthread_mutex_lock( &m_mutex );
   m_nCurrentWaitTime = nMS;
   pthread_mutex_unlock( &m_mutex );

   return true;
}

void* FlowdockManager::FlowdockManagerThread(void* ptr)
{
   FlowdockManager* pThis = (FlowdockManager*)ptr;
   pThis->DoWork();

   return NULL;
}

void FlowdockManager::DoWork()
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
      GetListenMessages();
      pthread_mutex_unlock( &m_mutex );
   }
}

void FlowdockManager::DoQueuedMessages()
{
   while( m_arrQueuedMessages.size() > 0 )
   {
      FlowdockQueuedMessage msg = m_arrQueuedMessages[0];

      if( msg.m_eType == FlowdockQueuedMessage::SayMessage )
      {
         FlowdockSayOrgFlowMessageFunc Say = (FlowdockSayOrgFlowMessageFunc)m_libraryFlowdock.Resolve("FlowdockSayOrgFlowMessage");
         if( Say )
         {
            std::string strTemp = URLEncode(msg.m_strMessage);
            int nMessageID = -1;
            Say(m_FlowdockInstance, msg.m_strOrg.c_str(), msg.m_strFlow.c_str(), strTemp.c_str());
         }
      }
      else if( msg.m_eType == FlowdockQueuedMessage::UploadFile )
      {
         FlowdockUploadFileFunc Upload = (FlowdockUploadFileFunc)m_libraryFlowdock.Resolve("FlowdockUploadFile");
         if( Upload )
         {
            std::string strFile = msg.m_strMessage;
            int nMessageID = -1;

            //Upload(m_arrFlowdockInstances[i], strFile.c_str());
         }
      }
      else if( msg.m_eType == FlowdockQueuedMessage::RestartCamp )
      {
         FlowdockFreeFunc FreeAPI = (FlowdockFreeFunc)m_libraryFlowdock.Resolve("FlowdockFree");
         if( FreeAPI )
            FreeAPI(&m_FlowdockInstance);

         //Rejoin(Flowdock_CAMP, Flowdock_AUTH, msg.m_strRoom, Flowdock_USESSL);
      }
      else
      {

      }

      m_arrQueuedMessages.erase(m_arrQueuedMessages.begin());
   }
}

void FlowdockManager::GetListenMessages()
{
   if( m_FlowdockInstance == NULL )
      return;

   FlowdockGetListenMessageCountFunc GetMessagesCount = (FlowdockGetListenMessageCountFunc)m_libraryFlowdock.Resolve("FlowdockGetListenMessageCount");
   FlowdockGetListenMessageTypeFunc GetMessageType = (FlowdockGetListenMessageTypeFunc)m_libraryFlowdock.Resolve("FlowdockGetListenMessageType");
   FlowdockGetMessageContentFunc GetMessageContent = (FlowdockGetMessageContentFunc)m_libraryFlowdock.Resolve("FlowdockGetMessageContent");
   FlowdockRemoveListenMessageFunc RemoveMessage = (FlowdockRemoveListenMessageFunc)m_libraryFlowdock.Resolve("FlowdockRemoveListenMessage");

   while(GetMessagesCount(m_FlowdockInstance) > 0 )
   {
      if( GetMessageType(m_FlowdockInstance, 0) == 0 ) {
         char strBuffer[1024*100];
         int nSizeOfMessage = 1024*100;

         if( 1 == GetMessageContent(m_FlowdockInstance, 0, strBuffer, nSizeOfMessage) )
         {
            int nType = 0;
            int nUserID = 0;

            m_pManager->MessageSaid("Org", "Room", 0/*type*/, 0/*userid*/, strBuffer);
         }
      }

      RemoveMessage(m_FlowdockInstance, 0);
   }
}

bool FlowdockManager::AddFlow(const std::string& strOrg, const std::string& strFlow)
{
   m_arrFlows.push_back(FlowdockFlowInfo(strOrg, strFlow));
   return true;
}

bool FlowdockManager::Connect(const std::string& strUsername, const std::string& strPassword)
{
   return Rejoin(strUsername, strPassword);
}

bool FlowdockManager::Rejoin(const std::string& strUsername, const std::string& strPassword)
{
   bool bOK = false;

   FlowdockCreateFunc CreateAPI = (FlowdockCreateFunc)m_libraryFlowdock.Resolve("FlowdockCreate");
   FlowdockAPI pFlowdock = NULL;

   FlowdockSetUsernamePasswordFunc SetDefaults = (FlowdockSetUsernamePasswordFunc)m_libraryFlowdock.Resolve("FlowdockSetUsernamePassword");
   if( !SetDefaults )
      goto Exit;

   FlowdockStartListeningDefaultsFunc Listen = (FlowdockStartListeningDefaultsFunc)m_libraryFlowdock.Resolve("FlowdockStartListeningDefaults");
   FlowdockAddListenFlowFunc AddListen = (FlowdockAddListenFlowFunc)m_libraryFlowdock.Resolve("FlowdockAddListenFlow");

   if( !CreateAPI )
      goto Exit;

   CreateAPI(&pFlowdock);

   SetDefaults(pFlowdock, strUsername.c_str(), strPassword.c_str());

   for(std::vector<FlowdockFlowInfo>::size_type i=0; i<m_arrFlows.size(); i++) {
      AddListen(pFlowdock, m_arrFlows[i].m_strOrg.c_str(), m_arrFlows[i].m_strFlow.c_str());
   }

   Listen(pFlowdock);

   m_FlowdockInstance = pFlowdock;

   bOK = true;

Exit:
   return bOK;
}

