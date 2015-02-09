#include "CommandProcessor.h"
#include <iostream>
#include <algorithm>
#include "ConnectionManager.h"
#ifdef _WIN32
#include <conio.h>
#endif

using namespace std;

int StringToInt(const std::string& str)
{
   return atoi(str.c_str());
}

std::string Trim(std::string str)
{
   while(str.size()>0 && (str[0]==' ' || str[str.size()-1]==' '))
   {
      if( str[0]==' ' )
         str.erase(0, 1);
      if( str[str.size()-1]==' ' )
         str.erase(str.size()-1, 1);
   }
   return str;
}

CommandProcessor::CommandProcessor(ConnectionManager* pConnectionManager)
: m_pConnectionManager(pConnectionManager)
{

}

void CommandProcessor::ParseCommandLine(int argc, char *argv[])
{
   for(int i=0; i<argc; i++)
   {
      std::string str = argv[i];

      if( str == "--join" )
         m_pConnectionManager->JoinRoom(Flowdock_ORG, argv[i+1]);

      if( str == "--reload" )
         m_pConnectionManager->ReloadChatHandlers();

      if( str == "--connect" )
         m_pConnectionManager->Connect(Flowdock_USERNAME, Flowdock_PASSWORD);

      if( str == "--verbose" )
         m_pConnectionManager->SetVerbose();
   }
}

void CommandProcessor::Run()
{
   DisplayHelp();

   while(true)
   {
      cout << "> ";
      std::string strCommand;
#ifdef _WIN32
      while (1){
         if ( kbhit() )
         {
            char key_code = getch();
            if( key_code == '\r')
            {
               cout << endl;
               break;
            }
            else if( key_code == '\b' )
            {
               cout << key_code;
               if( strCommand.length() > 0)
                  strCommand = strCommand.substr(0, strCommand.length()-1);
            }
            else
            {
               cout << key_code;
               strCommand.append(1, key_code);
            }
         }
         else
         {
            std::string strResponse;
            if( m_pConnectionManager->GetResponce(strResponse) )
            {
               cout << strResponse;
               cout << "> ";
            }
         }
      }
#else
      getline(cin, strCommand);
#endif

      if( IsExitCommand(strCommand) )
      {
         m_pConnectionManager->Exit();
         return;
      }

      else if( IsHelpCommand(strCommand) )
      {
         DisplayHelp();
      }

      else if( IsReloadHandlersCommand(strCommand) )
      {
         m_pConnectionManager->ReloadChatHandlers();
      }

      else if( IsListHandersCommand(strCommand) )
      {
         m_pConnectionManager->ListChatHandlers();
      }

      else if( IsDebugMessagesCommand(strCommand) )
      {
         m_pConnectionManager->ToggleDebugMessages();
      }

      else if( IsJoinCommand(strCommand) )
      {
         string strRoomName = Trim(strCommand.substr(4));
         m_pConnectionManager->JoinRoom(Flowdock_ORG, strRoomName);
      }

      else if( IsConnectCommand(strCommand) )
      {
         m_pConnectionManager->Connect(Flowdock_USERNAME, Flowdock_PASSWORD);
      }

      else if( IsSayCommand(strCommand) )
      {
         string strMessage = strCommand.substr(4);

         string::size_type nPos = strMessage.find_first_of(';');
         if( nPos == string::npos )
            return;

         string strRoom = Trim(strMessage.substr(0, nPos));
         strMessage = Trim(strMessage.substr(nPos+1));

         m_pConnectionManager->Say(Flowdock_ORG, strRoom, strMessage, -1/*CommentTo*/);
      }

#ifndef _WIN32
      std::string strResponse;
      if( m_pConnectionManager->GetResponce(strResponse) )
      {
         cout << strResponse;
         cout << "> ";
      }
#endif
   }
}

bool CommandProcessor::IsExitCommand(const std::string& strCommand) const
{
   return ToLower(strCommand) == "exit";
}

bool CommandProcessor::IsHelpCommand(const std::string& strCommand) const
{
   return ToLower(strCommand) == "help";
}

bool CommandProcessor::IsReloadHandlersCommand(const std::string& strCommand) const
{
   return ToLower(strCommand) == "reload";
}

bool CommandProcessor::IsListHandersCommand(const std::string& strCommand) const
{
   return ToLower(strCommand) == "listhandlers";
}

bool CommandProcessor::IsDebugMessagesCommand(const std::string& strCommand) const
{
   return ToLower(strCommand) == "debugmessages";
}

bool CommandProcessor::IsJoinCommand(const std::string& strCommand) const
{
   return ToLower(strCommand).find("join") == 0;
}

bool CommandProcessor::IsConnectCommand(const std::string& strCommand) const
{
   return ToLower(strCommand).find("connect") == 0;
}

bool CommandProcessor::IsSayCommand(const std::string& strCommand) const
{
   return ToLower(strCommand).find("say ") == 0;
}

std::string CommandProcessor::ToLower(const std::string& strCommand)
{
   std::string strReturn(strCommand);
   std::transform(strReturn.begin(), strReturn.end(), strReturn.begin(), ::tolower);
   return strReturn;
}

void CommandProcessor::DisplayHelp()
{
   cout << "******FlowdockBot******" << endl;
   cout << "Available commands:" << endl;
   cout << "Help - This help" << endl;
   cout << "Reload - Reloads the chat handlers" << endl;
   cout << "ListHandlers - Lists the chat handlers" << endl;
   cout << "DebugMessages - Toggle displaying messages" << endl;
   cout << "Join <RoomName> - Adds a room to join to" << endl;
   cout << "Connect - Connect to the rooms" << endl;
   cout << "Say <RoomName>;<text> - Say something" << endl;
   cout << "Exit - Closes the program" << endl;
}



