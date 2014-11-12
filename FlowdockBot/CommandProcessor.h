#pragma once
#include <string>

class ConnectionManager;

class CommandProcessor
{
public:
   CommandProcessor(ConnectionManager* pConnectionManager);

   void ParseCommandLine(int argc, char *argv[]);
   void Run();

protected:
   bool IsExitCommand(const std::string& strCommand) const;
   bool IsHelpCommand(const std::string& strCommand) const;
   bool IsReloadHandlersCommand(const std::string& strCommand) const;
   bool IsListHandersCommand(const std::string& strCommand) const;
   bool IsDebugMessagesCommand(const std::string& strCommand) const;
   bool IsJoinCommand(const std::string& strCommand) const;
   bool IsConnectCommand(const std::string& strCommand) const;
   bool IsSayCommand(const std::string& strCommand) const;

   static std::string ToLower(const std::string& strCommand);

   void DisplayHelp();

protected:
   ConnectionManager* m_pConnectionManager;//Does not own
};
