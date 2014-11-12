#include <iostream>
#include <string>
#include <fstream>
#include <pthread.h>

#include "ConnectionManager.h"
#include "CommandProcessor.h"

using namespace std;

int main(int argc, char *argv[])
{
   cout << "Starting up FlowdockBot" << endl;

   {
      ConnectionManager connections;

      CommandProcessor commandProcessor(&connections);

      commandProcessor.ParseCommandLine(argc, argv);

      commandProcessor.Run();
   }

   cout << "Exiting FlowdockBot" << endl;

	return 0;
}

