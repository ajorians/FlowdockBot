#ifndef FlowdockBOT_HANDLERAPI_H
#define FlowdockBOT_HANDLERAPI_H

#include <string>

class IHandler
{
public:
   virtual void SayMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrMessage, int nCommentTo, const char* pstrTags) = 0;
   virtual void UploadMessage(const char* pstrOrg, const char* pstrRoom, const char* pstrFile) = 0;
};

#ifdef WIN32
#define HANDLER_EXTERN	extern "C" __declspec(dllexport)
#else
#define HANDLER_EXTERN extern "C"
#endif

typedef int (*HandlerMessageSaidFunc)(void* pIFlowdockManager, const char* pstrOrg, const char* pstrFlow, int nType, int nUserID, const char* pstrMessage);

HANDLER_EXTERN int HandlerMessageSaid(void* pIFlowdockManager, const char* pstrOrg, const char* pstrFlow, int nType, int nUserID, const char* pstrMessage);

#endif
