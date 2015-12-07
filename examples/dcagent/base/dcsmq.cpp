#include "dcsmq.h"
#include "dcutils.hpp"
#include "profile.h"
#include <sys/msg.h>

struct dcsmq_t {
	dcsmq_config_t	conf;
	int				sender;
	int				recver;
	dcsmq_msg_cb_t	msg_cb;
	void		*	msg_cb_ud;
	msgbuf		*	sendbuff;
	msgbuf		*	recvbuff;
	uint64_t		session;	//myself id , send and recv msg cookie. in servermode is 0.
	dcsmq_t(){
		init();
	}
	void init(){
		sender = recver = -1;
		msg_cb = nullptr;
		sendbuff = recvbuff = nullptr;
		msg_cb_ud = nullptr;
		session = 0;
	}
};
int		_msgq_create(key_t key, int flag, size_t max_size){
	int id = msgget(key, flag);
	if (id < 0){
		//get error
		GLOG(LOG_LVL_ERROR, "msg get error !");
		return -1;
	}
	struct msqid_ds mds;
	int ret = msgctl(id, IPC_STAT, (struct msqid_ds *)&mds);
	if (ret != 0){
		GLOG(LOG_LVL_ERROR, "msgctl error !");
		return -2;
	}

	if (mds.msg_qbytes != max_size){
		mds.msg_qbytes = max_size;
		ret = msgctl(id, IPC_SET, (struct msqid_ds *)&mds);
		if (ret != 0){
			GLOG(LOG_LVL_ERROR, "msgctl error !");
			return -3;
		}
	}
	return id;
}

dcsmq_t * dcsmq_create(const dcsmq_config_t & conf){
	//sender : recver (1:2) : client
	int prj_id[] = { 1, 2 };
	if (conf.server_mode){
		prj_id[0] = 2;
		prj_id[1] = 1;
	}
	int flag = 0666;
	if (!conf.attach){
		flag |= IPC_CREAT;
	}
	key_t key = ftok(conf.key.c_str(), prj_id[0]);
	if (key == -1){
		//error no
		GLOG(LOG_LVL_ERROR, "ftok error key:%s , prj_id:%d",
			conf.key.c_str(), prj_id[0]);
		return nullptr;
	}
	int sender = _msgq_create(key, flag, conf.max_queue_buff_size);
	if (sender < 0){
		//errno
		GLOG(LOG_LVL_ERROR, "create msgq sender error flag :%d buff size:%u",
			flag, conf.max_queue_buff_size);
		return nullptr;
	}
	GLOG_TRA("create sender with key:%s(%d) , prj_id:%d",
		conf.key.c_str(), key, prj_id[0]);

	key = ftok(conf.key.c_str(), prj_id[1]);
	int recver = _msgq_create(key, flag, conf.max_queue_buff_size);
	if (recver < 0){
		//errno
		GLOG(LOG_LVL_ERROR, "create msgq recver error flag :%d buff size:%u",
			flag, conf.max_queue_buff_size);
		return nullptr;
	}
	GLOG_TRA("create recver with key:%s(%d) , prj_id:%d",
		conf.key.c_str(), key, prj_id[1]);

	dcsmq_t * smq = new dcsmq_t();
	if (!smq){
		//memalloc
		GLOG(LOG_LVL_FATAL, "malloc error");
		return nullptr;
	}
	smq->sendbuff = (msgbuf	*)malloc(conf.msg_buffsz);
	smq->recvbuff = (msgbuf	*)malloc(conf.msg_buffsz);
	if (!smq->sendbuff ||
		!smq->recvbuff){
		//mem alloc
		GLOG(LOG_LVL_FATAL, "malloc error");
		return nullptr;
	}
	smq->conf = conf;
	smq->sender = sender;
	smq->recver = recver;
	return smq;
}
void    dcsmq_destroy(dcsmq_t* smq){
	if (smq->sender >= 0){
		//no need
	}
	if (smq->recver >= 0){
		//no need
	}
	if (smq->sendbuff){
		free(smq->sendbuff);		
	}
	if (smq->recvbuff){
		free(smq->recvbuff);
	}
	smq->init();
	delete smq;
}
void    dcsmq_msg_cb(dcsmq_t * smq, dcsmq_msg_cb_t cb, void * ud){
	smq->msg_cb = cb;
	smq->msg_cb_ud = ud;
}
int     dcsmq_poll(dcsmq_t*  smq, int max_time_us){	
	PROFILE_FUNC();
	int64_t past_us = 0, start_us, now_us;
	start_us = dcsutil::time_unixtime_us();
	ssize_t msg_sz = 0;
	int nproc = 0, ntotal_proc = 0;
	while (past_us < max_time_us){
		msg_sz = msgrcv(smq->recver, smq->recvbuff, smq->conf.msg_buffsz, smq->session, IPC_NOWAIT);
		if (msg_sz <= 0){
			if (errno == EINTR){
				continue;
			}
			else
			if (errno == E2BIG){
				//error for msg too much , clear it then continue;
				msg_sz = msgrcv(smq->recver, smq->recvbuff, smq->conf.msg_buffsz, smq->session, IPC_NOWAIT | MSG_NOERROR);
				continue;
			}
			else if (errno != ENOMSG) {
				//error log for
			}
			break;
		}
		else {
			msgbuf * buf = (msgbuf*)(smq->recvbuff);
			GLOG_TRA("recv msgq msg from (type) :%lu size:%zu", buf->mtype, msg_sz);
			smq->msg_cb(smq, buf->mtype, dcsmq_msg_t(buf->mtext, msg_sz), smq->msg_cb_ud);
		}
		++nproc;
		++ntotal_proc;
		if (nproc >= 16){
			now_us = dcsutil::time_unixtime_us();
			past_us +=  (now_us - start_us);
			start_us = now_us;
			nproc = 0;
		}
	}
	return ntotal_proc;
}
int		dcsmq_push(dcsmq_t* smq, uint64_t dst, const dcsmq_msg_t & msg){
	if (msg.sz > smq->conf.msg_buffsz){
		return -1;
	}
	if (dst == 0){
		return -2;
	}
	smq->sendbuff->mtype = dst;
	memcpy(smq->sendbuff->mtext, msg.buffer, msg.sz);
	int ret = 0;
	do{
		ret = msgsnd(smq->recver, smq->sendbuff, msg.sz, IPC_NOWAIT);
	} while (ret == -1 && errno == EINTR);
	return ret;
}
int     dcsmq_send(dcsmq_t* smq, uint64_t dst, const dcsmq_msg_t & msg){
	if (msg.sz > smq->conf.msg_buffsz){
		//size error
		return -1;
	}
	if (dst == 0){
		//dst error
		return -2;
	}
	smq->sendbuff->mtype = dst;	
	memcpy(smq->sendbuff->mtext, msg.buffer, msg.sz);
	int ret = 0;
	do {
		ret = msgsnd(smq->sender, smq->sendbuff, msg.sz, IPC_NOWAIT);
	} while (ret == -1 && errno == EINTR);
	return ret;
}
bool	dcsmq_server_mode(dcsmq_t * smq){
	return smq->conf.server_mode;
}
uint64_t	dcsmq_session(dcsmq_t * smq){
	if (!smq) { return 0; }
	if (smq->conf.server_mode){ return 0; }
	return smq->session;
}
void	dcsmq_set_session(dcsmq_t * smq, uint64_t session){
	if (!smq->conf.server_mode){
		smq->session = session;
	}
}