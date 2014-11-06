#ifndef CAMPFIREBOT_HANDLERAPI_H
#define CAMPFIREBOT_HANDLERAPI_H

#include <string>

class IHandler
{
public:
   virtual void SayMessage(const char* pstrRoom, const char* pstrMessage) = 0;
   virtual void RestartCampfire(const char* pstrRoom)                   = 0;
   virtual void TrelloSubscribe(const char* pstrRoom, const char* pstrBoard, const char* pstrToken)   = 0;
   virtual void TrelloUnSubscribe(const char* pstrRoom, const char* pstrBoard) = 0;
};

#ifdef WIN32
#define HANDLER_EXTERN	extern "C" __declspec(dllexport)
#else
#define HANDLER_EXTERN extern "C"
#endif

//typedef int (*HandlerCreateFunc)(CampfireAPI* api);
//typedef int (*HandlerFreeFunc)(CampfireAPI* api);
typedef int (*HandlerMessageSaidFunc)(void* pICampfireManager, const char* pstrRoom, int nType, int nUserID, const char* pstrMessage);
typedef int (*HandlerTrelloAdjustmentFunc)(void* pICampfireManager, const char* pstrRoom, const char* pstrCard, const char* strListBefore, const char* strListAfter, const char* strDescription, int nCreated, int nClosed);
typedef int (*HandlerTimeEventFunc)(void* pICampfireManager, const char* pstrRoom);

//HANDLER_EXTERN int CampfireCreate(CampfireAPI* api);
//HANDLER_EXTERN int CampfireFree(CampfireAPI* api);
HANDLER_EXTERN int HandlerMessageSaid(void* pICampfireManager, const char* pstrRoom, int nType, int nUserID, const char* pstrMessage);
HANDLER_EXTERN int HandlerTrelloAdjustment(void* pICampfireManager, const char* pstrRoom, const char* pstrCard, const char* strListBefore, const char* strListAfter, const char* strDescription, int nCreated, int nClosed);
HANDLER_EXTERN int HandlerTimeEvent(void* pICampfireManager, const char* pstrRoom);

#endif
