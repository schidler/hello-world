#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

#define LISTEN_PORT           (8099)
#define LISTEN_BACKLOG        (32)
#define BIG_BUF_SIZE          (1024)

void do_accept_cb(evutil_socket_t listener, short event, void* arg);
void read_cb(struct bufferevent* bev, void* arg);
void error_cb(struct bufferevent* bev, short event, void* arg);
void write_cb(struct bufferevent* bev, void* arg);

int main(int argc, char* argv[])
{
	int ret = 0;
	evutil_socket_t listener;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > listener){
		printf("socket error, errno:%d, %s\n",
				errno, strerror(errno));
		return 1;
	}

	//设置端口可重用
	evutil_make_listen_socket_reuseable(listener);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(LISTEN_PORT);
	
	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0){
		printf("bind error, errno:%d, %s\n",
				errno, strerror(errno));
		return 1;
	}

	if (listen(listener, LISTEN_BACKLOG) < 0){
		printf("listen error, errno:%d, %s\n",
				errno, strerror(errno));
		return 1;
	}

	printf("serve on port:%d...\n", LISTEN_PORT);
	
	//设置为非阻塞模式
	evutil_make_socket_nonblocking(listener);

	//创建一个event实例
	struct event_base* ev_base = event_base_new();
	if (!ev_base){
		fprintf(stderr, "initial event_base failed.\n");
		return 1;
	}

	//创建并绑定一个event
	struct event* listen_ev;
	listen_ev = event_new(ev_base, listener, EV_READ | EV_PERSIST,
			do_accept_cb, (void*)ev_base);
	event_add(listen_ev, NULL);      //注册事件，NULL表示无超时设置
	event_base_dispatch(ev_base);    //程序进入无限循环，等待就绪事件并执行事件处理

	printf("serve end.");

	event_base_free(ev_base);
	ev_base = NULL;

	return 0;
}

void do_accept_cb(evutil_socket_t listener, short ev_flag, void* arg)
{
	struct event_base* ev_base = (struct event_base*)arg;
	struct bufferevent* bev;
	evutil_socket_t client_sock;
	struct sockaddr_in client_sock_addr;
	socklen_t slen;

	client_sock = accept(listener, (struct sockaddr*)&client_sock_addr, &slen);
	if (client_sock < 0){
		perror("accept");
		return;
	}

	printf("accept client_sock fd:%d\n", client_sock);
	
	//创建一个bufferevent实例，关联socket fd,托管给event_base实例
	//BEV_OPT_CLOSE_ON_FREE表示释放bufferevent时关闭底层传输端口。
	//这将关闭底层套接字，释放bufferevent等资源。
	bev = bufferevent_socket_new(ev_base, client_sock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, read_cb, NULL, error_cb, arg);
	//设置为可读、可写状态
	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);
}

void read_cb(struct bufferevent* bev, void* arg)
{
	char line[BIG_BUF_SIZE];
	int retbytes = 0;
	char res_str[] = "this is libevent server demo.\n";
	
	evutil_socket_t client_sock = bufferevent_getfd(bev);
	while ((retbytes = bufferevent_read(bev, line, BIG_BUF_SIZE)) > 0){
		line[retbytes] = '\0';
		printf("fd=%u, read line:%s\n", client_sock, line);

		bufferevent_write(bev, res_str, strlen(res_str) + 1);
	}
}

void write_cb(struct bufferevent* bev, void* arg)
{
	return;
}

void error_cb(struct bufferevent* bev, short ev_flag, void* arg)
{
	evutil_socket_t client_sock = bufferevent_getfd(bev);
	printf("fd = %d, ", client_sock);
	if (ev_flag & BEV_EVENT_TIMEOUT){
		printf("TIMEOUT\n");
	}
	else if (ev_flag & BEV_EVENT_EOF){
		printf("connection closed.\n");
	}
	else if (ev_flag & BEV_EVENT_ERROR){
		printf("some other error\n");
	}
	bufferevent_free(bev);
}
