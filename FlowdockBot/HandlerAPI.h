#ifndef FlowdockBOT_HANDLERAPI_H
#define FlowdockBOT_HANDLERAPI_H

#include <string>

class IHandler
{
public:
   virtual void SayMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrMessage) = 0;
   virtual void UploadMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrFile) = 0;
};

#ifdef WIN32
#define HANDLER_EXTERN	extern "C" __declspec(dllexport)
#else
#define HANDLER_EXTERN extern "C"
#endif

//typedef int (*HandlerCreateFunc)(FlowdockAPI* api);
//typedef int (*HandlerFreeFunc)(FlowdockAPI* api);
typedef int (*HandlerMessageSaidFunc)(void* pIFlowdockManager, const char* pstrRoom, int nType, int nUserID, const char* pstrMessage);
typedef int (*HandlerTrelloAdjustmentFunc)(void* pIFlowdockManager, const char* pstrRoom, const char* pstrCard, const char* strListBefore, const char* strListAfter, const char* strDescription, int nCreated, int nClosed);
typedef int (*HandlerTimeEventFunc)(void* pIFlowdockManager, const char* pstrRoom);

//HANDLER_EXTERN int FlowdockCreate(FlowdockAPI* api);
//HANDLER_EXTERN int FlowdockFree(FlowdockAPI* api);
HANDLER_EXTERN int HandlerMessageSaid(void* pIFlowdockManager, const char* pstrRoom, int nType, int nUserID, const char* pstrMessage);
HANDLER_EXTERN int HandlerTrelloAdjustment(void* pIFlowdockManager, const char* pstrRoom, const char* pstrCard, const char* strListBefore, const char* strListAfter, const char* strDescription, int nCreated, int nClosed);
HANDLER_EXTERN int HandlerTimeEvent(void* pIFlowdockManager, const char* pstrRoom);

#endif
