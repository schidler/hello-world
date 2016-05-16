#ifndef _EPOLL_CLIENT_H_
#define _EPOLL_CLIENT_H_

#define _MAX_SOCKFD_COUNT     (65535)
#define BIG_BUF_SIZE          (1024)

typedef enum _EPOLL_USER_STATUS_EM
{
	FREE = 0,
	CONNECT_OK = 1,
	SEND_OK = 2,
	RECV_OK = 3,
} EPOLL_USER_STATUS_EM;

typedef struct _UserStatus
{
	EPOLL_USER_STATUS_EM iUserStatus;
	int iSockFd;
	char cSendBuff[BIG_BUF_SIZE];
	int iBuffLen;
	unsigned int uEpollEvents;
} UserStatus;

class CEpollClient
{
	public:
		CEpollClient(int iUserCount, const char* pIp, int iPort);
		~CEpollClient();
		int RunFun();
	
	private:
		int ConnectToServer(int iUserId, const char* pServerIp, unsigned short uServerPort);
		int SendToServerData(int iUserId);
		int RecvFromServer(int iUserId, char* pRecvBuff, int iBuffLen);
		bool CloseUser(int iUserId);
		bool DelEpoll(int iSockFd);

	private:
		int m_iUserCount;
		UserStatus* m_pAllUserStatus;
		int m_iEpollFd;
		int m_iSockFd_UserId[_MAX_SOCKFD_COUNT];
		int m_iPort;
		char m_ip[100];
};

#endif

