#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <pthread.h>
#include <vector>
#include "FlowdockAPI.h"
#include "HandlerAPI.h"
#include "Library.h"

#if 1
#define Flowdock_ORG       "aj-org"
#define Flowdock_FLOW      "main"
#define Flowdock_USERNAME  "ajorians@gmail.com"
#define Flowdock_PASSWORD  "1Smajjmd"
#else
#define Flowdock_ORG       "techsmith"
#define Flowdock_FLOW      "camtasia-windows"
#define Flowdock_USERNAME  "a.orians@techsmith.com"
#define Flowdock_PASSWORD  "1Smajjmd"
#endif

class RLibrary;
class FlowdockManager;

class IManager
{
public:
   virtual void MessageSaid(const std::string& strOrg, const std::string& strFlow, int nType, int nUserID, const std::string& strMessage)  = 0;
};

class ConnectionManager : public IHandler, public IManager
{
public:
   ConnectionManager();
   ~ConnectionManager();

   //IHandler
   void SayMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrMessage, const char* pstrTags);
   void UploadMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrFile);

   //IManager
   void MessageSaid(const std::string& strOrg, const std::string& strFlow, int nType, int nUserID, const std::string& strMessage);

   //Commands
   void Exit();
   bool ReloadChatHandlers();
   bool ListChatHandlers();
   void ToggleDebugMessages();
   bool JoinRoom(const std::string& strOrg, const std::string& strFlow);
   bool Connect(const std::string& strUsername, const std::string& strPassword);
   bool Say(const std::string& strOrg, const std::string& strRoom, const std::string& strMessage);
   bool ChangeUpdateFrequency(int nMS);

   bool GetResponce(std::string& strResponse);

public:
   struct QueuedMessage
   {
      enum MessageType { SayMessage, UploadFile, JoinRoom, RestartCamp, ReloadHandlers, ListHandlers, AdjustUpdateFrequency };

      MessageType m_eType;
      std::string m_strOrg;
      std::string m_strFlow;
      std::string m_strMessage;
      std::string m_strTags;
      std::string m_strExternalUser;

      std::string strUsername;
      std::string strPassword;
      bool bSSL;

      int m_nAmount;
   };

protected:
   static void* ConnectionManagerThread(void* ptr);
   void DoWork();

   bool LoadChatHandlers();
   void DoQueuedMessages();

   bool Rejoin(const std::string& strUsername, const std::string& strPassword);

   void NotifyHandlers(const std::string& strOrg, const std::string& strFlow, int nType, int nUserID, const std::string& strMessage);

protected:
   pthread_t m_thread;
   pthread_mutex_t m_mutex;
   bool m_bExit;

   std::vector<QueuedMessage> m_arrQueuedMessages;

   pthread_mutex_t m_mutexResponses;
   std::vector<std::string> m_arrResponses;
   bool m_bDebugFlowdockMessages;

   std::vector<RLibrary*> m_arrChatHandlers;
   FlowdockManager* m_pFlowdockManager;
};

#endif
