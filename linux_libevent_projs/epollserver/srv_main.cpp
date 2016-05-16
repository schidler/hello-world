#include "epollserver.h"

int main()
{
	CEpollServer epollsrv;
	epollsrv.InitServer("127.0.0.1", 8099);
	epollsrv.Run();

	return 0;
}
