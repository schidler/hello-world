#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <event.h>
#include <evhttp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_ADDR              "0.0.0.0"
#define LISTEN_PORT              (12089)
#define THREAD_COUNT             (4)
#define BACKLOG                  (10240)

int httpsrv_bindsock(int port, int backlog);
int httpsrv_start(int port, int thread_count, int backlog);
void* httpsrv_dispatch(void* arg);
void httpsrv_generic_handle(struct evhttp_request* req, void* arg);
void httpsrv_process_req(struct evhttp_request* req);

int main(int argc, char* argv[])
{
	printf("http server start on %d...\n", LISTEN_PORT);
	if (0 > httpsrv_start(12089, 4, 10240))
		printf("http server start failed.\n");

	return 0;
}

int httpsrv_bindsock(int port, int backlog)
{
	int listen_sock = -1;
	int one = 1;
	struct sockaddr_in srv_addr;
	int flags = -1;

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > listen_sock){
		printf("create socket failed, %s\n", strerror(errno));
		return -1;
	}

	if (0 != setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
				(char*)&one, sizeof(one))){
		printf("setsockopt failed, %s\n", strerror(errno));
		return -1;
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr(LISTEN_ADDR);
	srv_addr.sin_port = htons(port);

	if (0 > bind(listen_sock, (struct sockaddr*)&srv_addr, sizeof(srv_addr))){
		printf("bind failed, %s\n", strerror(errno));
		return -1;
	}

	if (0 > listen(listen_sock, backlog)){
		printf("listen failed, %s\n", strerror(errno));
		return -1;
	}

	if (0 > (flags = fcntl(listen_sock, F_GETFL, 0))
			|| 0 > fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK)){
		printf("fcntl failed, %s\n", strerror(errno));
		return -1;
	}

	return listen_sock;
}

int httpsrv_start(int port, int thread_count, int backlog)
{
	int i = 0;
	int listen_sock = -1;
	pthread_t* ths = NULL;
	struct event_base** ev_base_list = NULL;
	struct evhttp** httpd_list = NULL;

	listen_sock = httpsrv_bindsock(port, backlog);
	if (0 > listen_sock)
		return -1;

	ths = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
	ev_base_list = (struct event_base**)malloc((sizeof(struct event_base*) * thread_count));
	httpd_list = (struct evhttp**)malloc(sizeof(struct evhttp*) * thread_count);
	if (NULL == ths || NULL == ev_base_list || NULL == httpd_list){
		printf("malloc failed, %s\n", strerror(errno));
		return -1;
	}

	for (i = 0; i < thread_count; i++){
		ev_base_list[i] = event_base_new();
		if (NULL == ev_base_list[i])
			return -1;

		httpd_list[i] = evhttp_new(ev_base_list[i]);
		if (NULL == httpd_list[i])
			return -1;

		if (0 != evhttp_accept_socket(httpd_list[i], listen_sock))
			return -1;

		evhttp_set_gencb(httpd_list[i], httpsrv_generic_handle, NULL);

		if (0 != pthread_create(&ths[i], NULL, 
					httpsrv_dispatch, ev_base_list[i]))
			return -1;
	}

	for (i = 0; i < thread_count; i++)
		pthread_join(ths[i], NULL);

	for (i = 0; i < thread_count; i++){
		if (NULL != ev_base_list[i]){
			event_base_free(ev_base_list[i]);
			ev_base_list[i] = NULL;
		}

		if (NULL != httpd_list[i]){
			evhttp_free(httpd_list[i]);
			httpd_list[i] = NULL;
		}
	}

	if (NULL != ev_base_list){
		free(ev_base_list);
		ev_base_list = NULL;
	}
	if (NULL != httpd_list){
		free(httpd_list);
		httpd_list = NULL;
	}
}

void* httpsrv_dispatch(void* arg)
{
	event_base_dispatch((struct event_base*)arg);
	return NULL;
}

void httpsrv_generic_handle(struct evhttp_request* req, void* arg)
{
	httpsrv_process_req(req);
}

void httpsrv_process_req(struct evhttp_request* req)
{
	struct evbuffer* buf = evbuffer_new();
	if (NULL == buf)
		return;

	evbuffer_add_printf(buf, "server response by thread %u, request uri:%s\n",
			pthread_self(), evhttp_request_get_uri(req));
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);
	buf = NULL;
}
