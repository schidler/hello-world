#include "epollclient.h"
#include <stdio.h>

int main()
{
    CEpollClient* epollclient = new CEpollClient(10000, "127.0.0.1", 8099);
	if (NULL == epollclient){
		printf("CEpollClient start failed!\n");
		return 0;
	}

	epollclient->RunFun();

	if (NULL != epollclient){
		delete epollclient;
		epollclient = NULL;
	}

	return 0;
}
