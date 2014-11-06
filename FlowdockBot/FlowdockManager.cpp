#include "CampfireManager.h"
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
#define CAMPFIRE_MODULE "CampfireAPI.dll"
#else
#define CAMPFIRE_MODULE "libCampfireAPI.so"
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

CampfireManager::CampfireManager(IManager* pManager)
: 
#ifdef WIN32
m_mutex(PTHREAD_MUTEX_INITIALIZER),
#endif
 m_libraryCampfire(GetSelfDirectory() + CAMPFIRE_MODULE
), m_pManager(pManager), m_bExit(false), m_nCurrentWaitTime(5000)
{
#ifndef WIN32
   pthread_mutex_init(&m_mutex, NULL);
#endif
   if( !m_libraryCampfire.Load() )
   {
      std::cout << "Couldn't load CampfireAPI" << std::endl;
      return;
   }
   int iRet;
   iRet = pthread_create( &m_thread, NULL, CampfireManager::CampfireManagerThread, (void*)this);
}

CampfireManager::~CampfireManager()
{
   Exit();
}

void CampfireManager::Exit()
{
   pthread_mutex_lock( &m_mutex );
   m_bExit = true;
   pthread_mutex_unlock( &m_mutex );

   pthread_join( m_thread, NULL);

   for(std::vector<CampfireAPI>::size_type i=m_arrCampfireInstances.size(); i-->0; )
   {
      CampfireAPI pCampfire = m_arrCampfireInstances[i];
      CampfireLeaveFunc Leave = (CampfireLeaveFunc)m_libraryCampfire.Resolve("CampfireLeave");
      if( Leave )
         Leave(pCampfire);

      CampfireFreeFunc FreeAPI = (CampfireFreeFunc)m_libraryCampfire.Resolve("CampfireFree");
      if( FreeAPI )
         FreeAPI(&pCampfire);

      m_arrCampfireInstances.erase(m_arrCampfireInstances.begin() + i);
      m_arrCampfireLoginInfo.erase(m_arrCampfireLoginInfo.begin() + i);
   }
}

bool CampfireManager::JoinRoom(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL)
{
   bool bOK = false;

   CampfireQueuedMessage msg;
   msg.m_eType = CampfireQueuedMessage::RestartCamp;
   msg.m_strRoom = strRoom;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return bOK;
}

bool CampfireManager::LeaveRoom(const std::string& strRoom)
{
   bool bOK = false;

   CampfireQueuedMessage msg;
   msg.m_eType = CampfireQueuedMessage::LeaveRoom;
   msg.m_strRoom = strRoom;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return bOK;
}

bool CampfireManager::Say(const std::string& strRoom, const std::string& strMessage)
{
   CampfireQueuedMessage msg;
   msg.m_eType = CampfireQueuedMessage::SayMessage;
   msg.m_strRoom = strRoom;
   msg.m_strMessage = strMessage;

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool CampfireManager::Upload(const std::string& strRoom, const std::string& strFile, bool bDelete)
{
   CampfireQueuedMessage msg;
   msg.m_eType = CampfireQueuedMessage::UploadFile;
   msg.m_strRoom = strRoom;
   msg.m_strMessage = strFile;
   if( bDelete )
      msg.m_strPassword = "Delete";

   pthread_mutex_lock( &m_mutex );
   m_arrQueuedMessages.push_back(msg);
   pthread_mutex_unlock( &m_mutex );

   return true;
}

bool CampfireManager::ChangeUpdateFrequency(int nMS)
{
   pthread_mutex_lock( &m_mutex );
   m_nCurrentWaitTime = nMS;
   pthread_mutex_unlock( &m_mutex );

   return true;
}

std::vector<std::string> CampfireManager::GetRoomList()
{
   std::vector<std::string> arrstrRooms;

   pthread_mutex_lock( &m_mutex );
   for(std::vector<CampfireLoginInfo>::size_type i=0; i<m_arrCampfireLoginInfo.size(); i++)
   {
      arrstrRooms.push_back(m_arrCampfireLoginInfo[i].m_strRoom);
   }
   pthread_mutex_unlock( &m_mutex );

   return arrstrRooms;
}

void* CampfireManager::CampfireManagerThread(void* ptr)
{
   CampfireManager* pThis = (CampfireManager*)ptr;
   pThis->DoWork();

   return NULL;
}

void CampfireManager::DoWork()
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

void CampfireManager::DoQueuedMessages()
{
   while( m_arrQueuedMessages.size() > 0 )
   {
      CampfireQueuedMessage msg = m_arrQueuedMessages[0];

      if( msg.m_eType == CampfireQueuedMessage::SayMessage )
      {
         if( m_arrCampfireInstances.size() >= 1 )
         {
            CampfireSayFunc Say = (CampfireSayFunc)m_libraryCampfire.Resolve("CampfireSay");
            if( Say )
            {
               std::string strTemp = URLEncode(msg.m_strMessage);
               int nMessageID = -1;
               for(std::vector<CampfireAPI>::size_type i=0; i<m_arrCampfireInstances.size(); i++)
               {
                  if( m_arrCampfireLoginInfo[i].m_strRoom != msg.m_strRoom )
                     continue;

                  Say(m_arrCampfireInstances[i], strTemp.c_str(), nMessageID);
               }
            }
         }
      }
      else if( msg.m_eType == CampfireQueuedMessage::UploadFile )
      {
         if( m_arrCampfireInstances.size() >= 1 )
         {
            CampfireUploadFileFunc Upload = (CampfireUploadFileFunc)m_libraryCampfire.Resolve("CampfireUploadFile");
            if( Upload )
            {
               std::string strFile = msg.m_strMessage;
               int nMessageID = -1;
               for(std::vector<CampfireAPI>::size_type i=0; i<m_arrCampfireInstances.size(); i++)
               {
                  if( m_arrCampfireLoginInfo[i].m_strRoom != msg.m_strRoom )
                     continue;

                  Upload(m_arrCampfireInstances[i], strFile.c_str());

                  if( msg.m_strPassword == "Delete" )
                  {
#ifdef _WIN32
                     DeleteFileA(strFile.c_str());
#else
                     unlink(strFile.c_str());
#endif
                  }
               }
            }
         }
      }
      else if( msg.m_eType == CampfireQueuedMessage::RestartCamp )
      {
         if( m_arrCampfireInstances.size() > 0 )
         {
            for(std::vector<CampfireAPI>::size_type i=0; i<m_arrCampfireInstances.size(); i++)
            {
               CampfireAPI pCampfire = m_arrCampfireInstances[i];
               CampfireLoginInfo loginInfo = m_arrCampfireLoginInfo[i];

               if( loginInfo.m_strRoom != msg.m_strRoom )
                  continue;

               m_arrCampfireInstances.erase(m_arrCampfireInstances.begin() + i);
               m_arrCampfireLoginInfo.erase(m_arrCampfireLoginInfo.begin() + i);
               CampfireFreeFunc FreeAPI = (CampfireFreeFunc)m_libraryCampfire.Resolve("CampfireFree");
               if( FreeAPI )
                  FreeAPI(&pCampfire);
            }
         }

         Rejoin(CAMPFIRE_CAMP, CAMPFIRE_AUTH, msg.m_strRoom, CAMPFIRE_USESSL);
      }
      else if( msg.m_eType == CampfireQueuedMessage::LeaveRoom )
      {
         if( m_arrCampfireInstances.size() > 0 )
         {
            for(std::vector<CampfireAPI>::size_type i=0; i<m_arrCampfireInstances.size(); i++)
            {
               CampfireAPI pCampfire = m_arrCampfireInstances[i];
               CampfireLoginInfo loginInfo = m_arrCampfireLoginInfo[i];

               if( loginInfo.m_strRoom != msg.m_strRoom )
                  continue;

               CampfireLeaveFunc Leave = (CampfireLeaveFunc)m_libraryCampfire.Resolve("CampfireLeave");
               if( Leave )
                  Leave(pCampfire);

               m_arrCampfireInstances.erase(m_arrCampfireInstances.begin() + i);
               m_arrCampfireLoginInfo.erase(m_arrCampfireLoginInfo.begin() + i);
               CampfireFreeFunc FreeAPI = (CampfireFreeFunc)m_libraryCampfire.Resolve("CampfireFree");
               if( FreeAPI )
                  FreeAPI(&pCampfire);
            }
         }
      }
      else
      {

      }

      m_arrQueuedMessages.erase(m_arrQueuedMessages.begin());
   }
}

void CampfireManager::GetListenMessages()
{
   CampfireGetListenMessageFunc GetListenMessage = (CampfireGetListenMessageFunc)m_libraryCampfire.Resolve("CampfireGetListenMessage");

   if( m_arrCampfireInstances.size() <= 0 )
      return;

   for(std::vector<CampfireAPI>::size_type j=0; j<m_arrCampfireInstances.size(); j++)
   {
      CampfireAPI pCampfire = m_arrCampfireInstances[j];

      char strBuffer[1024*100];
      int nSizeOfMessage = 1024*100;
      if( 1 == GetListenMessage(pCampfire, strBuffer, nSizeOfMessage) )
      {
         int nType = 0;
         int nUserID = 0;

         m_pManager->MessageSaid(m_arrCampfireLoginInfo[j].m_strRoom, nType, nUserID, strBuffer);
      }
   }
}

bool CampfireManager::Rejoin(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL)
{
   //std::cout << "Rejoin called" << std::endl;
   bool bOK = false;

   CampfireCreateFunc CreateAPI = (CampfireCreateFunc)m_libraryCampfire.Resolve("CampfireCreate");
   CampfireAPI pCampfire = NULL;

   CampfireLoginFunc Login = (CampfireLoginFunc)m_libraryCampfire.Resolve("CampfireLogin");
   CampfireRoomsCountFunc GetRoomCount = (CampfireRoomsCountFunc)m_libraryCampfire.Resolve("CampfireRoomsCount");
   CampfireGetRoomNameIDFunc GetRoomName = (CampfireGetRoomNameIDFunc)m_libraryCampfire.Resolve("CampfireGetRoomNameID");
   CampfireJoinRoomFunc JoinRoom = (CampfireJoinRoomFunc)m_libraryCampfire.Resolve("CampfireJoinRoom");
   CampfireListenFunc Listen = (CampfireListenFunc)m_libraryCampfire.Resolve("CampfireListen");
   bool bLoggedOn = false;
   int nRoomCount = -1; 
   int nRoomNum = 0;

   if( !CreateAPI )
      goto Exit;

   CreateAPI(&pCampfire);

   if( !Login )
      goto Exit;

   bLoggedOn = Login(pCampfire, strCamp.c_str(), strAuth.c_str(), bSSL) == 1;

   if( !GetRoomCount )
      goto Exit;

   GetRoomCount(pCampfire, nRoomCount);

   if( !GetRoomName )
      goto Exit;

   for(nRoomNum=0; nRoomNum<nRoomCount; nRoomNum++)
   {
      char * pstrRoomName;
      pstrRoomName = NULL;
      int nRoomID = 0;

      int nSizeOfRoomName = 0, nSizeOfRoomURL = 0;
      GetRoomName(pCampfire, nRoomNum, pstrRoomName, nSizeOfRoomName, nRoomID);

      pstrRoomName = new char[nSizeOfRoomName + 1];

      GetRoomName(pCampfire, nRoomNum, pstrRoomName, nSizeOfRoomName, nRoomID);

      std::string strRoomName(pstrRoomName);

      delete[] pstrRoomName;

      if( strRoomName == strRoom )
         break;
   }

   if( nRoomNum >= nRoomCount )
   {
      std::cout << "Room not found" << std::endl;
      goto Exit;
   }

   if( !JoinRoom )
      goto Exit;

   JoinRoom(pCampfire, nRoomNum);

   Listen(pCampfire);

   m_arrCampfireInstances.push_back(pCampfire);
   m_arrCampfireLoginInfo.push_back(CampfireLoginInfo(strRoom));

   bOK = true;

Exit:
   return bOK;
}

