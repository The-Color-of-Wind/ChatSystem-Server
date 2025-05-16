
#include <iostream>

#include "./MainReactor/MainReactor.h"
using namespace std;

int main()
{
	IniConfig::getInstance().iniConfigData();
	MainReactor* mainReactor = new MainReactor(12345, 3);
	mainReactor->run();

	return 0;
}


