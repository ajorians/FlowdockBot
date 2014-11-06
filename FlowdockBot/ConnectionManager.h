#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <pthread.h>
#include <vector>
#include "CampfireAPI.h"
#include "HandlerAPI.h"
#include "Library.h"

//#if 1
//#define CAMPFIRE_CAMP   "testwith"
//#define CAMPFIRE_AUTH   "534c7a1597bfb3c862de90f69310a8ad1a1eda04"
//#define CAMPFIRE_USESSL true
//#else
#define CAMPFIRE_CAMP   "camtasiaslate"
#define CAMPFIRE_AUTH   "0c2f691569082638c0469f52dabfa5a0fdb29b6e"
#define CAMPFIRE_USESSL true
//#endif

class RLibrary;
class CampfireManager;

class IManager
{
public:
   virtual void MessageSaid(const std::string& strRoom, int nType, int nUserID, const std::string& strMessage)  = 0;
   virtual void TelloUpdate(const std::string& strRoom, const std::string& strName, const std::string& strListBefore, const std::string& strListAfter, const std::string& strDescription, bool bCreated, bool bClosed) = 0;
};

class ConnectionManager : public IHandler, public IManager
{
public:
   ConnectionManager();
   ~ConnectionManager();

   //IHandler
   void SayMessage(const char* pstrRoom, const char* pstrMessage);
   void RestartCampfire(const char* pstrRoom);
   void TrelloSubscribe(const char* pstrRoom, const char* pstrBoard, const char* pstrToken);
   void TrelloUnSubscribe(const char* pstrRoom, const char* pstrBoard);
   void UploadMessage(const char* pstrRoom, const char* pstrFile, bool bDelete);

   //IManager
   void MessageSaid(const std::string& strRoom, int nType, int nUserID, const std::string& strMessage);
   void TelloUpdate(const std::string& strRoom, const std::string& strName, const std::string& strListBefore, const std::string& strListAfter, const std::string& strDescription, bool bCreated, bool bClosed);

   //Commands
   void Exit();
   bool ReloadChatHandlers();
   bool ListChatHandlers();
   void ToggleDebugMessages();
   bool JoinRoom(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL);
   bool LeaveRoom(const std::string& strRoom);
   bool Say(const std::string& strRoom, const std::string& strMessage);
   bool ChangeUpdateFrequency(int nMS);
   bool StartTrelloUpdate(const std::string& strRoom);

   bool GetResponce(std::string& strResponse);

public:
   struct QueuedMessage
   {
      enum MessageType { SayMessage, UploadFile, RestartCamp, RestartTrello, ReloadHandlers, ListHandlers, AdjustUpdateFrequency, TrelloSubscribe, TrelloUnSubscribe, LeaveRoom };

      MessageType m_eType;
      std::string m_strRoom;
      std::string m_strMessage;
      std::string m_strToken;

      std::string strCamp;
      std::string strUsername;
      std::string strPassword;
      std::string strRoom;
      bool bSSL;

      int m_nAmount;
   };

protected:
   static void* ConnectionManagerThread(void* ptr);
   void DoWork();

   bool LoadChatHandlers();
   void DoQueuedMessages();
   void DoTimedEvents(const std::vector<std::string>& arrRooms);

   bool Rejoin(const std::string& strCamp, const std::string& strAuth, const std::string& strRoom, bool bSSL);
   bool Leave(const std::string& strRoom);

   void NotifyHandlers(const std::string& strRoom, int nType, int nUserID, const std::string& strMessage);
   void NotifyHandlers(const std::string& strRoom, const std::string& strName, const std::string& strListBefore, const std::string& strListAfter, const std::string& strDescription, bool bCreated, bool bClosed);

protected:
   pthread_t m_thread;
   pthread_mutex_t m_mutex;
   bool m_bExit;

   std::vector<QueuedMessage> m_arrQueuedMessages;

   pthread_mutex_t m_mutexResponses;
   std::vector<std::string> m_arrResponses;
   bool m_bDebugCampfireMessages;

   std::vector<RLibrary*> m_arrChatHandlers;
   CampfireManager* m_pCampfireManager;
};

#endif
