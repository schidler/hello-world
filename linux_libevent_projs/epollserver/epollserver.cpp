#include "epollserver.h"
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define _MAX_SOCKFD_COUNT        (65536)
#define BIG_BUF_SIZE             (1024)

CEpollServer::CEpollServer()
{
}

CEpollServer::~CEpollServer()
{
	close(m_isock);
}

bool CEpollServer::InitServer(const char* chIp, int iPort)
{
	m_iEpollFd = epoll_create(_MAX_SOCKFD_COUNT);
	
	int opts = O_NONBLOCK;
	if (fcntl(m_iEpollFd, F_SETFL, opts) < 0){
		printf("设置非阻塞模式失败！\n");
		return false;
	}

	m_isock = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > m_isock){
		printf("socket error, errno:%d,%s\n",
				errno, strerror(errno));
	}

	sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(iPort);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_addr.s_addr = inet_addr(chIp);

	int ireuseadd_on = 1;     //支持端口复用
	setsockopt(m_isock, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on));

	if (bind(m_isock, (sockaddr*)&listen_addr, sizeof(listen_addr)) != 0){
		printf("bind error, errno:%d, %s\n",
				errno, strerror(errno));
		return false;
	}

	if (listen(m_isock, 20) < 0){
		printf("listen error, errno:%d, %s\n",
				errno, strerror(errno));
		return false;
	}
	else{
		printf("服务端监听中...\n");
	}

	if (pthread_create(&m_ListenThreadId, 0, (void* (*)(void*))ListenThread, this) != 0){
		printf("create server listen thread failed, errno:%d, %s\n",
				errno, strerror(errno));
		return false;
	}
}

void CEpollServer::ListenThread(void* lpVoid)
{
	CEpollServer* pEpollSrv = (CEpollServer*)lpVoid;
	sockaddr_in remote_addr;
	int len = sizeof(remote_addr);

	while(true){
		int client_sock = accept(pEpollSrv->m_isock, 
				(sockaddr*)&remote_addr, (socklen_t*)&len);
		if (client_sock < 0){
			printf("server accept failed, errno:%d, %s\n",
					errno, strerror(errno));
			continue;
		}
		else{
			struct epoll_event ev;
			ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
			ev.data.fd = client_sock;
			epoll_ctl(pEpollSrv->m_iEpollFd, EPOLL_CTL_ADD,
					client_sock, &ev);
		}
	}
}

void CEpollServer::Run()
{
	while(true){
		struct epoll_event events[_MAX_SOCKFD_COUNT];
		int nfds = epoll_wait(m_iEpollFd, events, _MAX_SOCKFD_COUNT, -1);
        printf("epoll_wait count nfds:%d\n", nfds);
		for (int i = 0; i < nfds; i++){
			int client_socket = events[i].data.fd;
			char buffer[BIG_BUF_SIZE];
			memset(buffer, 0, BIG_BUF_SIZE);
			if (events[i].events & EPOLLIN){
				//监听到读事件，则接收数据
				int rev_size = recv(events[i].data.fd, buffer,
						BIG_BUF_SIZE, 0);
				if (0 > rev_size){
					printf("rev error, errno:%d, %s, rev_size=%d\n",
							errno, strerror(errno), rev_size);
					struct epoll_event event_del;
					event_del.data.fd = events[i].data.fd;
					event_del.events = 0;
					epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL,
							event_del.data.fd, &event_del);
				}
				else{
					printf("Terminal received msg content:%s\n", buffer);
					struct epoll_event ev;
					ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
					ev.data.fd = client_socket;
					epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD,
							client_socket, &ev);
				}
			}
			else if (events[i].events & EPOLLOUT){
                time_t nowtime;
                struct tm* timeinfo;
                time(&nowtime);
                timeinfo = localtime(&nowtime);

				char send_buf[BIG_BUF_SIZE];
                sprintf(send_buf, "%4d-%2d-%2d-%2d-%2d-%d:\tclient socket fd:%d\n",
                        timeinfo->tm_year + 1900,
                        timeinfo->tm_mon + 1,
                        timeinfo->tm_mday,
                        timeinfo->tm_hour,
                        timeinfo->tm_min,
                        timeinfo->tm_sec,
                        client_socket);
				int sendsize = send(client_socket, send_buf, 
						strlen(send_buf) + 1, MSG_NOSIGNAL);
				if (0 >= sendsize){
					printf("send error, errno:%d, %s, sendsize=%d\n",
							errno, strerror(errno), sendsize);
					struct epoll_event event_del;
					event_del.data.fd = events[i].data.fd;
					event_del.events = 0;
					epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL,
							event_del.data.fd, &event_del);
				}
				else{
					printf("server send reply msg ok, msg:%s\n", send_buf);
					struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
					ev.data.fd = client_socket;
					epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD,
							client_socket, &ev);
				}
			}
			else{
				printf("epoll error\n");
				epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL,
						events[i].data.fd, &events[i]);
			}
		}
	}
}
