#pragma once
#include "stdinc.h"

struct dcsmq_t;

struct dcsmq_config_t
{
    string			key;
    int				msg_buffsz;
	int				max_queue_buff_size;
	bool			server_mode;
	bool			attach;
	dcsmq_config_t()
	{
		msg_buffsz = 1024 * 1024;
		max_queue_buff_size = 10 * 1024 * 1024; //10MB
		server_mode = false;
		attach = false;
	}
};

struct dcsmq_msg_t
{
    const char * buffer;
    int			 sz;
	dcsmq_msg_t(const char * buf, int s) :buffer(buf), sz(s){}
};

typedef int (*dcsmq_msg_cb_t)(dcsmq_t * , uint64_t src, const dcsmq_msg_t & msg, void * ud);
dcsmq_t *		dcsmq_create(const dcsmq_config_t & conf);
void		dcsmq_destroy(dcsmq_t*);
void		dcsmq_msg_cb(dcsmq_t *, dcsmq_msg_cb_t cb, void * ud);
int			dcsmq_poll(dcsmq_t*, int max_time_us );
int			dcsmq_send(dcsmq_t*,uint64_t dst, const dcsmq_msg_t & msg);
int			dcsmq_push(dcsmq_t*, uint64_t dst, const dcsmq_msg_t & msg);//send to peer like himself
bool		dcsmq_server_mode(dcsmq_t *);
void		dcsmq_set_session(dcsmq_t *, uint64_t session); //send or recv type
uint64_t	dcsmq_session(dcsmq_t *);
//for debug
//status report

