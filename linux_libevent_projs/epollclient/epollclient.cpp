#include "epollclient.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

CEpollClient::CEpollClient(int iUserCount, const char* pIp, int iPort)
{
	strcpy(m_ip, pIp);
	m_iPort = iPort;
	m_iUserCount = iUserCount;
	m_iEpollFd = epoll_create(_MAX_SOCKFD_COUNT);
	m_pAllUserStatus = (UserStatus*)malloc(sizeof(UserStatus) * iUserCount);
	for (int iuserid = 0; iuserid < iUserCount; ++iuserid){
		m_pAllUserStatus[iuserid].iUserStatus = FREE;
        sprintf(m_pAllUserStatus[iuserid].cSendBuff, "Client user id:%d send msg \"hello server\"\n to epollserver", iuserid);
		m_pAllUserStatus[iuserid].iBuffLen = strlen(m_pAllUserStatus[iuserid].cSendBuff) + 1;
		m_pAllUserStatus[iuserid].iSockFd = -1;
	}
	memset(m_iSockFd_UserId, 0xFF, sizeof(m_iSockFd_UserId));
}

CEpollClient::~CEpollClient()
{
	if (NULL != m_pAllUserStatus){
		free(m_pAllUserStatus);
		m_pAllUserStatus = NULL;
	}
}

int CEpollClient::ConnectToServer(int iUserId, const char* pServerIp, unsigned short uServerPort)
{
	m_pAllUserStatus[iUserId].iSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_pAllUserStatus[iUserId].iSockFd < 0){
		printf("[CEPollClient error]:init socket fail, userid:%d, errno:%d, %s\n",
				iUserId, errno, strerror(errno));
		m_pAllUserStatus[iUserId].iSockFd = -1;
		return m_pAllUserStatus[iUserId].iSockFd;
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(uServerPort);
	addr.sin_addr.s_addr = inet_addr(pServerIp);

	//支持端口复用
	int ireuseadd_on = 1;      
	setsockopt(m_pAllUserStatus[iUserId].iSockFd, SOL_SOCKET,
			SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on));

	if (connect(m_pAllUserStatus[iUserId].iSockFd, (const sockaddr*)&addr, sizeof(addr)) < 0){
		printf("[CEPollClient error]:connect fail, userid:%d, errno:%d, %s\n",
				iUserId, errno, strerror(errno));
		this->CloseUser(iUserId);
		return m_pAllUserStatus[iUserId].iSockFd;
	}
	m_pAllUserStatus[iUserId].iUserStatus = CONNECT_OK;
	m_pAllUserStatus[iUserId].iSockFd = m_pAllUserStatus[iUserId].iSockFd;

	//设置为非阻塞模式
	unsigned long ul = 1;
	ioctl(m_pAllUserStatus[iUserId].iSockFd, FIONBIO, &ul);

	return m_pAllUserStatus[iUserId].iSockFd;
}

int CEpollClient::SendToServerData(int iUserId)
{
    //usleep(1);

	int isendsize = -1;
	if (CONNECT_OK == m_pAllUserStatus[iUserId].iUserStatus
			|| RECV_OK == m_pAllUserStatus[iUserId].iUserStatus){
		isendsize = send(m_pAllUserStatus[iUserId].iSockFd, 
				m_pAllUserStatus[iUserId].cSendBuff, m_pAllUserStatus[iUserId].iBuffLen, MSG_NOSIGNAL);
		if (isendsize < 0){
			printf("[CEPollClient error]:SendToServerData, send fail, userid:%d, errno:%d, %s\n",
					iUserId, errno, strerror(errno));
		}
		else{
			printf("[CEPollClient info]:SendToServerData, userid:%d send msg content:%s",
					m_pAllUserStatus[iUserId].cSendBuff);
			m_pAllUserStatus[iUserId].iUserStatus = SEND_OK;
		}
	}

	return isendsize;
}

int CEpollClient::RecvFromServer(int iUserId, char* pRecvBuff, int iBuffLen)
{
	int irecvsize = -1;
	if (SEND_OK == m_pAllUserStatus[iUserId].iUserStatus){
		irecvsize = recv(m_pAllUserStatus[iUserId].iSockFd, pRecvBuff, iBuffLen, 0);
		if (0 > irecvsize){
			printf("[CEPollClient error]:RecvFromServer, recv fail, userid:%d, errno:%d, %s\n",
					iUserId, errno, strerror(errno));
		}
		else if (0 == irecvsize){
			printf("[CEPollClient warning]:RecvFromServer, STB recv data length is 0, disconnected, userid:%d, iSockFd:%d\n",
					iUserId, m_pAllUserStatus[iUserId].iSockFd);
		}
		else{
            time_t nowtime;
            struct tm* timeinfo;
            time(&nowtime);
            timeinfo = localtime(&nowtime);

            printf("%4d-%2d-%2d-%2d-%2d-%d:\t[CEPollClient info]:RecvFromServer, userid:%d recv from server msg:%s\n",
                   timeinfo->tm_year + 1900,
                   timeinfo->tm_mon + 1,
                   timeinfo->tm_mday,
                   timeinfo->tm_hour,
                   timeinfo->tm_min,
                   timeinfo->tm_sec,
                   iUserId,
                   pRecvBuff);
			m_pAllUserStatus[iUserId].iUserStatus = RECV_OK;
		}
	}

	return irecvsize;
}

bool CEpollClient::CloseUser(int iUserId)
{
	close(m_pAllUserStatus[iUserId].iSockFd);
	m_pAllUserStatus[iUserId].iUserStatus = FREE;
	m_pAllUserStatus[iUserId].iSockFd = -1;
	return true;
}

int CEpollClient::RunFun()
{
	int isockfd = -1;
	for (int iuserid = 0; iuserid < m_iUserCount; ++iuserid){
		struct epoll_event event;
		isockfd = this->ConnectToServer(iuserid, m_ip, m_iPort);
		if (0 > isockfd){
			printf("[CEPollClient error]:RunFun, connect fail\n");
		}
		m_iSockFd_UserId[isockfd] = iuserid;       //将用户id和sockfd关联起来

		event.data.fd = isockfd;
		event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;

		m_pAllUserStatus[iuserid].uEpollEvents = event.events;
		epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, event.data.fd, &event);
	}

	while(true){
		struct epoll_event events[_MAX_SOCKFD_COUNT];
		char buffer[BIG_BUF_SIZE];
		memset(buffer, 0, BIG_BUF_SIZE);
		int nfds = epoll_wait(m_iEpollFd, events, _MAX_SOCKFD_COUNT, 100); //等待epoll事件产生
		for (int ifd = 0; ifd < nfds; ++ifd){
			struct epoll_event event_nfds;
			int iclientsockfd = events[ifd].data.fd;
			printf("events[%d].data.fd:%d\n", ifd, iclientsockfd);
			int iuserid = m_iSockFd_UserId[iclientsockfd];
			if (events[ifd].events & EPOLLOUT){
				int iret = this->SendToServerData(iuserid);
				if (0 < iret){
					event_nfds.events = EPOLLIN | EPOLLERR | EPOLLHUP;
					event_nfds.data.fd = iclientsockfd;
					epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, event_nfds.data.fd, &event_nfds);
				}
				else{
					printf("[CEPollClient error]:EpollWait, SnedToServerData fail, userid:%d, send iret:%d\n", 
							iuserid, iret);
					this->DelEpoll(events[ifd].data.fd);
					this->CloseUser(iuserid);
				}
			}
			else if (events[ifd].events & EPOLLIN){
				//监听到读事件，则接收数据
				int ilen = this->RecvFromServer(iuserid, buffer, BIG_BUF_SIZE);
				if (0 > ilen){
					printf("[CEPollClient error]:RunFun, RecvFromServer fail, userid:%d, recv ilen:%d\n", 
							iuserid, ilen);
					this->DelEpoll(events[ifd].data.fd);
					this->CloseUser(iuserid);
				}
				else if(0 == ilen){
					printf("[CEPollClient warning]:RunFun, server disconnected, userid:%d, recv ilen:%d\n", 
							iuserid, ilen);
					this->DelEpoll(events[ifd].data.fd);
					this->CloseUser(iuserid);
				}
				else{
					m_iSockFd_UserId[iclientsockfd] = iuserid;
					event_nfds.data.fd = iclientsockfd;
					event_nfds.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
					epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, event_nfds.data.fd, &event_nfds);
				}
			}
			else{
				printf("[CEPollClient error]:RunFun, Unkown error\n");
				this->DelEpoll(events[ifd].data.fd);
				this->CloseUser(iuserid);
			}
		}
	}
}

bool CEpollClient::DelEpoll(int iSockFd)
{
	bool bret = false;
	struct epoll_event ev_del;
	if (0 < iSockFd){
		ev_del.data.fd = iSockFd;
		ev_del.events = 0;
		if (0 == epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, ev_del.data.fd, &ev_del)){
			bret = true;
		}
		else{
			printf("[CEPollClient error]:DelEpoll, epoll_ctl fail, iSockFd:%d\n", iSockFd);
		}
		m_iSockFd_UserId[iSockFd] = -1;
	}
	else{
		bret = true;
	}

	return bret;
}
