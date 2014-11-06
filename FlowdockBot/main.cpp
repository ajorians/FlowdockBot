#include <iostream>
#include <string>
#include <fstream>
#include <pthread.h>

#include "ConnectionManager.h"
#include "CommandProcessor.h"

using namespace std;

int main(int argc, char *argv[])
{
   cout << "Starting up CampfireBot" << endl;

   {
      ConnectionManager connections;

      CommandProcessor(&connections).Run();
   }

   cout << "Exiting CampfireBot" << endl;

	return 0;
}

