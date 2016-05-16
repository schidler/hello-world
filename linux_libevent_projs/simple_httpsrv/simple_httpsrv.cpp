#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <stdio.h>

#define LISTEN_PORT           (12088)
#define HTTPSRV_ADDR          "0.0.0.0"

static void generic_handle(struct evhttp_request* req, void* arg) 
{
	struct evbuffer* buf = evbuffer_new();
	if (!buf) {
		puts("failed to create reponse buffer\r\n");
		return;
	}

	evbuffer_add_printf(buf, "server response. requested:%s\r\n",
			evhttp_request_get_uri(req));
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);
}

int run_simple_httpsrv(void)
{
	struct event_base* ev_base = event_base_new();
	struct evhttp* httpsrv = evhttp_new(ev_base);
	if (!httpsrv) {
		printf("evhttp_new failed.\r\n");
		return -1;
	}

	int ret = evhttp_bind_socket(httpsrv, HTTPSRV_ADDR, LISTEN_PORT);
	if (0 != ret) {
		printf("evhttp_bind_socket failed(%.8X)", ret);
		return -1;
	}

	evhttp_set_gencb(httpsrv, generic_handle, NULL);

	printf("http serve start on %d...\r\n", LISTEN_PORT);

	event_base_dispatch(ev_base);

	evhttp_free(httpsrv);

	return 0;
}

int main()
{
	int ret = run_simple_httpsrv();

	return ret;
}
