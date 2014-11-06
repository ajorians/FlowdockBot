#pragma once

#include <pthread.h>
#include <vector>
#include <string>
#include "CampfireAPI.h"
#include "HandlerAPI.h"
#include "Library.h"

class RLibrary;
class IManager;

struct CampfireLoginInfo
{
   CampfireLoginInfo(const std::string& strRoom)
      : m_strRoom(strRoom)
   {}
   CampfireLoginInfo(const CampfireLoginInfo& rhs)
   {
      m_strRoom = rhs.m_strRoom;
   }
   std::string m_strRoom;
};

class CampfireManager
{
public:
   CampfireManager(IManager* pManager);
   ~CampfireManager();

   //Commands
   void Exit();
   bool JoinRoom(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL);
   bool LeaveRoom(const std::string& strRoom);
   bool Say(const std::string& strRoom, const std::string& strMessage);
   bool Upload(const std::string& strRoom, const std::string& strFile, bool bDelete);
   bool ChangeUpdateFrequency(int nMS);

   //Notify Commands
   /*void NotifySay(const std::string& strMessage);*/

   std::vector<std::string> GetRoomList();

public:
   struct CampfireQueuedMessage
   {
      enum MessageType { SayMessage, UploadFile, RestartCamp, LeaveRoom };

      MessageType m_eType;
      std::string m_strMessage;
      std::string m_strRoom;
      std::string m_strUsername;
      std::string m_strPassword;
   };

protected:
   static void* CampfireManagerThread(void* ptr);
   void DoWork();

   void DoQueuedMessages();
   void GetListenMessages();

   bool Rejoin(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL);

protected:
   pthread_t m_thread;
   pthread_mutex_t m_mutex;
   bool m_bExit;

   int m_nCurrentWaitTime;

   IManager* m_pManager;

   std::vector<CampfireQueuedMessage> m_arrQueuedMessages;

   RLibrary m_libraryCampfire;
   std::vector<CampfireAPI> m_arrCampfireInstances;
   std::vector<CampfireLoginInfo> m_arrCampfireLoginInfo;
};



