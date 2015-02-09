#pragma once

#include <pthread.h>
#include <vector>
#include <string>
#include "FlowdockAPI.h"
#include "HandlerAPI.h"
#include "Library.h"

class RLibrary;
class IManager;

struct FlowdockFlowInfo
{
   FlowdockFlowInfo(const std::string& strOrg, const std::string& strFlow)
      : m_strOrg(strOrg), m_strFlow(strFlow)
   {}
   FlowdockFlowInfo(const FlowdockFlowInfo& rhs)
   {
      m_strOrg = rhs.m_strOrg;
      m_strFlow = rhs.m_strFlow;
   }
   std::string m_strOrg;
   std::string m_strFlow;
};

class FlowdockManager
{
public:
   FlowdockManager(IManager* pManager);
   ~FlowdockManager();

   //Commands
   void Exit();
   bool AddFlow(const std::string& strOrg, const std::string& strFlow);
   bool Connect(const std::string& strUsername, const std::string& strPassword, bool bVerbose);
   bool Say(const std::string& strOrg, const std::string& strFlow, const std::string& strMessage, int nCommentTo, const std::string& strTags, const std::string& strExternalUser);
   bool Upload(const std::string& strRoom, const std::string& strFile);
   bool ChangeUpdateFrequency(int nMS);

   //Notify Commands
   /*void NotifySay(const std::string& strMessage);*/

public:
   struct FlowdockQueuedMessage
   {
      enum MessageType { SayMessage, UploadFile, RestartCamp };

      MessageType m_eType;
      std::string m_strMessage;
      std::string m_strOrg;
      std::string m_strFlow;
      std::string m_strUsername;
      std::string m_strPassword;
      int m_nCommentTo;
      std::string m_strTags;
      std::string m_strExternalUser;
   };

protected:
   static void* FlowdockManagerThread(void* ptr);
   void DoWork();

   void DoQueuedMessages();
   void GetListenMessages();

   bool Rejoin(const std::string& strUsername, const std::string& strPassword, bool bVerbose);

protected:
   pthread_t m_thread;
   pthread_mutex_t m_mutex;
   bool m_bExit;

   int m_nCurrentWaitTime;

   IManager* m_pManager;

   std::vector<FlowdockQueuedMessage> m_arrQueuedMessages;

   RLibrary m_libraryFlowdock;
   FlowdockAPI m_FlowdockInstance;
   std::vector<FlowdockFlowInfo> m_arrFlows;
};



