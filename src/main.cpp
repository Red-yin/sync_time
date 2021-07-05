#include "time_sync.h"
#include <string>
#include <iostream>

using namespace std;
int main()
{
	TimeSync *t = new TimeSync();
	string str;
	while(1){
		printf("cli>\n");
		cin>>str;
		if(str == "wakeup"){
			cout << "send WAKEUP event\n";
			t->send(WAKEUP);
		}
	}
	return 0;
}
