#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <evhtp.h>

int dump_header(evhtp_kv_t *kv, void *arg)
{
    char *buf = (char *)arg;
    int len = strlen(buf);
    len += snprintf(buf + len, 1024-len, "%s:%s\n", kv->key, kv->val);
    buf[len] = ' ';
    return 0;
}

void testcb(evhtp_request_t *request, void *a) 
{
    if (NULL == request->uri->authority) { 
		printf("authority is null\n");
	}

    printf("request method is %d\n", evhtp_request_get_method(request));
    printf("uri is %s\n", request->uri->path->full);
    printf("content length is %d\n", evhtp_request_content_len(request));
    char buf[1024] = {0};
    evbuffer_copyout(request->buffer_in, buf, evhtp_request_content_len(request));
    buf[evhtp_request_content_len(request)] = ' ';
    printf("content is %s\n", buf);

    printf("ext_system values is %s\n", evhtp_kv_find(request->headers_in, "fields")); 
    char mybuf[1024];
    mybuf[0] = ' ';
    evhtp_kvs_for_each(request->headers_in, dump_header, (void *)mybuf);
    printf("header details:%s\n", mybuf);

	char rsp_msg[] = "server has gotten your request\n";
    evbuffer_add_reference(request->buffer_out, rsp_msg, strlen(rsp_msg), NULL, NULL);
    evhtp_send_reply(request, EVHTP_RES_OK);
}

char g_easy_rsp_msg[] = "easy test\n";
void easy_handle_req_cb(evhtp_request_t *req, void *arg)
{
	time_t tmt = {0};
	struct tm *t = NULL;
	tmt = time(NULL);
	t = localtime(&tmt);

	char rsp_buf[1024] = {0};
	sprintf(rsp_buf, "%s ==> %s\n", asctime(t), g_easy_rsp_msg);

	//printf("%s\n", rsp_buf);

	evbuffer_add_reference(req->buffer_out, rsp_buf, strlen(rsp_buf), NULL, NULL);
	evhtp_send_reply(req, EVHTP_RES_OK);
}

int
main(int argc, char ** argv) {
    evbase_t * evbase = event_base_new();
    evhtp_t  * htp    = evhtp_new(evbase, NULL);

    evhtp_set_cb(htp, "/simple/", testcb, "simple");
    evhtp_set_cb(htp, "/1/ping", testcb, "one");
    evhtp_set_cb(htp, "/1/ping.json", testcb, "two");
	evhtp_set_cb(htp, "/easytest/", easy_handle_req_cb, NULL);
#if 1
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, NULL, 4, NULL);
#endif
#endif
    evhtp_bind_socket(htp, "0.0.0.0", 8081, 1024);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);

    return 0;
}

