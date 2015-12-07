#pragma once
#include "stdinc.h"

struct dctcp_msg_t
{
    const char * buff;
    int   buff_sz;
	dctcp_msg_t(const char * buf, int sz) :buff(buf), buff_sz(sz){}
};

struct dctcp_addr_t
{
    string ip;
    int  port;
	dctcp_addr_t() :ip("0.0.0.0"), port(0) {}
	uint32_t u32ip() const
	{
		return inet_addr(ip.c_str());
	}
};
struct dctcp_config_t
{
    int server_mode; //0:client, 1:server
    int max_recv_buff;
    int max_send_buff;
	int max_backlog;
	int max_client;
	int max_tcp_send_buff_size;
	int max_tcp_recv_buff_size;
	dctcp_config_t()
	{
		max_client = 8192;
		max_backlog = 2048;
		max_recv_buff = max_send_buff = 1024 * 100; //100K
		max_tcp_send_buff_size = 1024 * 100; //100K
		max_tcp_recv_buff_size = 640 * 100; //600K
		server_mode = 0;
	}
};

struct dctcp_t;
enum dctcp_close_reason_type
{
	DCTCP_MSG_OK = 0, //OK
	DCTCP_MSG_ERR = 1,	//msg error
	DCTCP_CONNECT_ERR = 2, //connect
	DCTCP_PEER_CLOSE = 3,
	DCTCP_POLL_ERR = 4, //refer to errno
	DCTCP_INVAL_CALL = 5, //usage err
	DCTCP_SYS_ERR = 6, //system call error refer to errno
	DCTCP_CLOSE_ACTIVE = 7, //by uplayer
};

enum dctcp_event_type
{
	DCTCP_EVT_INIT = 0,
    DCTCP_CONNECTED = 1,
	DCTCP_NEW_CONNX,
	DCTCP_CLOSED ,
    DCTCP_READ ,
    DCTCP_WRITE ,
    DCTCP_EVENT_MAX
};

struct dctcp_event_t
{
    dctcp_event_type			type;
	int							listenfd;//for new connection host fd
	int							fd;//event fd
    const dctcp_msg_t *			msg;
	dctcp_close_reason_type		reason;
	int							error;
	dctcp_event_t() :type(DCTCP_EVT_INIT), listenfd(-1),fd(-1),msg(nullptr), reason(DCTCP_MSG_OK), error(0){}
};

typedef int (*dctcp_event_cb_t)(dctcp_t*, const dctcp_event_t & ev, void * ud);

struct dctcp_t *	dctcp_create(const dctcp_config_t & conf);
void				dctcp_destroy(dctcp_t * );
void				dctcp_event_cb(dctcp_t*, dctcp_event_cb_t cb, void *ud);
//return proced events
int					dctcp_poll(dctcp_t *, int timeout_us, int max_proc = 100);
int					dctcp_listen(dctcp_t *, const dctcp_addr_t & addr); //return a fd >= 0when success
int					dctcp_connect(dctcp_t *, const dctcp_addr_t & addr, int retry = 0);
int					dctcp_reconnect(dctcp_t* , int fd);
int					dctcp_send(dctcp_t *, int fd, const dctcp_msg_t & msg);
void				dctcp_close(dctcp_t *, int fd);


