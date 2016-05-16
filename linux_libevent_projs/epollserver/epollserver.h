#ifndef _EPOLL_SERVER_H_
#define _EPOLL_SERVER_H_

#include <pthread.h>

class CEpollServer
{
	public:
		CEpollServer();
		~CEpollServer();

		bool InitServer(const char* chIp, int iPort);
		void Listen();
		static void ListenThread(void* lpVoid);
		void Run();

	private:
		int m_iEpollFd;
		int m_isock;
		pthread_t m_ListenThreadId;
};

#endif

