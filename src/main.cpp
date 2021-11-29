#include "time_sync.h"
#include <string>
#include <iostream>
extern "C"{
	#include "slog.h"
};

using namespace std;
int main()
{
	TimeSync *t = new TimeSync();
	string str;
	while(1){
		printf("cli>\n");
		str.clear();
		cin>>str;
		dbg("%s", str.c_str());
		if(str == "wakeup"){
			cout << "send WAKEUP event\n";
		}
	}
	return 0;
}
