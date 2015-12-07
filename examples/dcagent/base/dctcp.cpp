#include "dctcp.h"
#include "msg_proto.hpp"
#include "logger.h"
#include "profile.h"

//typedef	std::shared_ptr<msg_buffer_t>	msg_buffer_ptr_t;

//client
struct dctcp_connecting_t {
	int				reconnect;
	int				max_reconnect;
	sockaddr_in 	connect_addr;
	dctcp_connecting_t() {
		bzero(this, sizeof(*this));
	}
};
struct dctcp_listener_t {
	sockaddr_in			listenaddr;
	dctcp_listener_t(){
		bzero(this, sizeof(*this));
	}
};


struct dctcp_t {
	dctcp_config_t	conf;
	int				epfd;	//epfd for poller
	std::unordered_map<int, dctcp_connecting_t>		connectings;//client connecting
	std::unordered_map<int, dctcp_listener_t>		listeners;//server listeners fd
	///////////////////////////////
	dctcp_event_cb_t	event_cb;
	void		*		event_cb_ud;
	///////////////////////////////
	epoll_event	*	events;
	int				nproc;
	int				nevts;
	//////////////////////////////fd<->send and recv
	std::unordered_map<int, msg_buffer_t>	sock_recv_buffer;
	std::unordered_map<int, msg_buffer_t>	sock_send_buffer;
	////////////////////////////////////////
	msg_buffer_t		misc_buffer;
	dctcp_t(){
		init();
	}
	void init(){
		epfd = -1;
		event_cb = nullptr;
		event_cb_ud = nullptr;
		events = nullptr;
		nevts = nproc = 0;
		sock_recv_buffer.clear();
		sock_send_buffer.clear();
		connectings.clear();
		listeners.clear();
		misc_buffer.destroy();
	}
};

static int _set_socket_opt(int fd, int name, void * val, socklen_t len){
	int lv = SOL_SOCKET;
	if (name == TCP_NODELAY) lv = IPPROTO_TCP;
	return setsockopt(fd, lv, name, val, len);
}

static int _set_socket_ctl(int fd, int flag, bool open){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0){
		return -1;
	}
	if (open){
		flags |= flag;
	}
	else{
		flags &= ~(flag);
	}
	if (fcntl(fd, F_SETFL, flags) < 0){
		return -1;
	}
	return 0;
}
static int 	_init_socket_options(int fd, int sendbuffsize,int recvbuffsize){
	int on = 1;
	int ret = _set_socket_opt(fd, TCP_NODELAY, &on, sizeof(on));
	ret |= _set_socket_opt(fd, SO_REUSEADDR, &on, sizeof(on));
	size_t buffsz = recvbuffsize;
	ret |= _set_socket_opt(fd, SO_RCVBUF, &buffsz, sizeof(buffsz));
	buffsz = sendbuffsize;
	ret |= _set_socket_opt(fd, SO_SNDBUF, &buffsz, sizeof(buffsz));
	ret |= _set_socket_ctl(fd, O_NONBLOCK, true); //nblock
	if (ret != 0){
		close(fd);
		return ret;
	}
	return 0;
}
static int _create_tcpsocket(int sendbuffsize, int recvbuffsize){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;
	int ret = _init_socket_options(fd , sendbuffsize, recvbuffsize);
	if (ret) return -2;
	return fd;
}

struct dctcp_t * dctcp_create(const dctcp_config_t & conf){
	dctcp_t * stcp = new dctcp_t();
	stcp->conf = conf;
	stcp->events = (epoll_event *)malloc(conf.max_client * sizeof(epoll_event));
	if (!stcp->events) { dctcp_destroy(stcp); return nullptr; }

	stcp->epfd = epoll_create(conf.max_client);
	if (stcp->epfd < 0) { dctcp_destroy(stcp); return nullptr; }

	stcp->misc_buffer.create(1024 * 1024);
	return stcp;
}
inline	void	_close_listeners(dctcp_t * stcp){
	for (auto & it : stcp->listeners) close(it.first);
}
inline	void	_close_connections(dctcp_t * stcp){
	for (auto & it : stcp->connectings) close(it.first);
}
void            dctcp_destroy(dctcp_t * stcp){
	GLOG_TRA("stcp destroy ....");
	_close_listeners(stcp);
	_close_connections(stcp);
	if (stcp->epfd >= 0) close(stcp->epfd);
	if (stcp->events) free(stcp->events);
	stcp->init();
	delete stcp;
}
void            dctcp_event_cb(dctcp_t* stcp, dctcp_event_cb_t cb, void * ud){
	stcp->event_cb = cb;
	stcp->event_cb_ud = ud;
}
static int _op_poll(dctcp_t * stcp, int cmd, int fd, int flag = 0){
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = flag;
	return epoll_ctl(stcp->epfd, cmd, fd, &ev);
}
static int _get_sockerror(int fd){
	int error = 0;
	socklen_t len = sizeof(int);
	int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
	if (ret == 0)
		return error;
	else 
		return errno;
}

static void _free_sock_msg_buffer(dctcp_t * stcp, int fd){
	auto it = stcp->sock_recv_buffer.find(fd);
	if (it != stcp->sock_recv_buffer.end()){
		GLOG_TRA("free recv buffer ....fd:%d", fd);
		it->second.destroy();
		stcp->sock_recv_buffer.erase(it);
	}
	it = stcp->sock_send_buffer.find(fd);
	if (it != stcp->sock_send_buffer.end()){
		GLOG_TRA("free send buffer ....fd:%d", fd);
		it->second.destroy();
		stcp->sock_send_buffer.erase(it);
	}
}
static msg_buffer_t * _get_sock_msg_buffer(dctcp_t * stcp, int fd, bool for_recv){
	if (for_recv){
		auto it = stcp->sock_recv_buffer.find(fd);
		if (it == stcp->sock_recv_buffer.end()){
			msg_buffer_t buf;
			if (buf.create(stcp->conf.max_recv_buff)){
				return nullptr;
			}
			stcp->sock_recv_buffer[fd] = buf;
			return &(stcp->sock_recv_buffer[fd]);
		}
		return &(it->second);
	}
	else{
		auto it = stcp->sock_send_buffer.find(fd);
		if (it == stcp->sock_send_buffer.end()){
			msg_buffer_t	buf;
			if (buf.create(stcp->conf.max_send_buff)){
				return nullptr;
			}
			stcp->sock_send_buffer[fd] = buf;
			return &(stcp->sock_send_buffer[fd]);
		}
		return &(it->second);
	}
}
static void	_close_fd(dctcp_t * stcp, int fd, dctcp_close_reason_type reason){
	GLOG_TRA("close fd:%d for reason:%d", fd, reason);
	int error = _get_sockerror(fd);
	_op_poll(stcp, EPOLL_CTL_DEL, fd);
	close(fd);
	//connection ?	//listener ?
	if (stcp->connectings.find(fd) != stcp->connectings.end()){
		stcp->connectings.erase(fd);
	}
	else if (stcp->listeners.find(fd) != stcp->listeners.end()){
		stcp->listeners.erase(fd);
	}
	_free_sock_msg_buffer(stcp, fd);
	//just notify
	dctcp_event_t  sev;
	sev.type = dctcp_event_type::DCTCP_CLOSED;
	sev.fd = fd;
	sev.reason = reason;
	sev.error = error;
	stcp->event_cb(stcp, sev, stcp->event_cb_ud);
}

static void _new_connx(dctcp_t * stcp, int listenfd){
	struct sockaddr	addr;
	socklen_t	len = sizeof(addr);
	int nfd = accept(listenfd, &addr, &len);
	if (nfd >= 0){
		dctcp_event_t sev;
		sev.listenfd = listenfd;
		sev.fd = nfd;
		sev.type = dctcp_event_type::DCTCP_NEW_CONNX;
		_init_socket_options(nfd, stcp->conf.max_tcp_send_buff_size, stcp->conf.max_tcp_recv_buff_size);
		//add epollin
		_op_poll(stcp, EPOLL_CTL_ADD, nfd, EPOLLIN);
		stcp->event_cb(stcp, sev, stcp->event_cb_ud);
	}
	else{		//error
		GLOG_TRA("accept error listen fd:%d for:%s", listenfd, strerror(errno));
	}
}

static int _read_msg_error(dctcp_t * stcp, int fd, int read_ret){
	if (read_ret == 0){
		//peer close
		_close_fd(stcp, fd, dctcp_close_reason_type::DCTCP_PEER_CLOSE);
		return -1;
	}
	else{ //if (sz < 0 ) 
		if (errno == EINTR) {
			return 0;
		}
		if (errno != EAGAIN &&
			errno != EWOULDBLOCK ){
			//error
			_close_fd(stcp, fd, dctcp_close_reason_type::DCTCP_SYS_ERR);
			return -2;
		}
		return -3;
	}
}
static int _dispatch_msg(dctcp_t * stcp, int fd, msg_buffer_t * buffer){
	dctcp_event_t	sev;
	sev.type = DCTCP_READ;
	sev.fd = fd;
	int nmsg = 0;
	int msg_buff_start = 0;
	int total = ntohl(*(int32_t*)(buffer->buffer));
	while ( msg_buff_start + total <= buffer->valid_size ) {
		dctcp_msg_t smsg(buffer->buffer + msg_buff_start + sizeof(int32_t),
			total - sizeof(int32_t));
		sev.msg = &smsg;
		stcp->event_cb(stcp, sev, stcp->event_cb_ud);
		nmsg++;
		msg_buff_start += total;
		total = 0;
		if (msg_buff_start + (int)sizeof(int32_t) <= buffer->valid_size){
			total = ntohl(*(int32_t*)(buffer->buffer + msg_buff_start));
		}
		else{
			break;
		}
	}
	if (buffer->valid_size > msg_buff_start){
		memmove(buffer->buffer,
			buffer->buffer + msg_buff_start,
			buffer->valid_size - msg_buff_start);
		GLOG_TRA("memmove size:%d", buffer->valid_size - msg_buff_start);
		buffer->valid_size -= msg_buff_start;
	}
	else {
		assert(buffer->valid_size == msg_buff_start);
		buffer->valid_size = 0;
	}
	return nmsg;
}

static int _read_tcp_socket(dctcp_t * stcp, int fd){
	msg_buffer_t * buffer = _get_sock_msg_buffer(stcp, fd, true);
	if (!buffer) {
		return -1;
	}
	int nmsg = 0;
	while (buffer->max_size > buffer->valid_size) {
		int sz = recv(fd, buffer->buffer + buffer->valid_size, 
						buffer->max_size - buffer->valid_size, MSG_DONTWAIT);
		if (sz > 0){
			buffer->valid_size += sz;
		}
		else if (sz == -1 && errno == EINTR){
			continue;
		}
		else if (_read_msg_error(stcp, fd, sz)){
			return -1;
		}
		//dispatch
		if (buffer->valid_size > (int)sizeof(int32_t)){
			int32_t total = ntohl(*(int32_t*)buffer->buffer);
			if (total > buffer->max_size){
				//errror msg , too big 
				_close_fd(stcp, fd, dctcp_close_reason_type::DCTCP_MSG_ERR);
				return -2;
			}
			else if (total > buffer->valid_size){
				//next time
				return 0;
			}
			else{
				//need dispatching
				nmsg += _dispatch_msg(stcp, fd, buffer);
			}
		}
	}
	return nmsg;
}
static void _connect_check(dctcp_t * stcp, int fd){
	int error = 0;
	auto it = stcp->connectings.find(fd);
	if (it == stcp->connectings.end()){
		//not connecting fd
		return ;
	}
	auto & cnx = it->second;
	socklen_t len = sizeof(int);
	if (0 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)){
		dctcp_event_t sev;
		sev.fd = fd;
		if (0 == error){
			//clear when connection established
			stcp->connectings.erase(fd);
			sev.type = dctcp_event_type::DCTCP_CONNECTED;
			stcp->event_cb(stcp, sev, stcp->event_cb_ud);
			_op_poll(stcp, EPOLL_CTL_MOD, fd, EPOLLIN);
			return;
		}
	}
	else {
		if (cnx.reconnect < cnx.max_reconnect){
			//reconnect
			_op_poll(stcp, EPOLL_CTL_DEL, fd);
			dctcp_reconnect(stcp, fd);
		}
		else{
			_close_fd(stcp, fd, dctcp_close_reason_type::DCTCP_CONNECT_ERR);
		}
	}
}

//write msg
static int _write_tcp_socket(dctcp_t * stcp, int fd, const char * msg, int sz){
	//write app buffer ? write tcp socket directly ?
	dctcp_event_t sev;
	sev.fd = fd;
	sev.type = dctcp_event_type::DCTCP_WRITE;

	int total = sz + sizeof(int32_t);
	if (total > stcp->conf.max_send_buff){
		return -1;
	}
	msg_buffer_t* msgbuff = _get_sock_msg_buffer(stcp, fd, false);
	if (!msgbuff){
		//no buffer
		return -2;
	}
	*(int32_t*)(msgbuff->buffer) = htonl(total);
	memcpy(msgbuff->buffer + sizeof(int32_t), msg, sz);
	
RETRY_WRITE_MSG:
	int ret = send(fd, msgbuff->buffer, total, MSG_DONTWAIT | MSG_NOSIGNAL);
	if (ret == total){
		//send ok
		dctcp_msg_t smsg = dctcp_msg_t(msg, sz);
		sev.msg = &smsg;
		stcp->event_cb(stcp, sev, stcp->event_cb_ud);
		return 0;
	}
	else {
		if ( ret == -1 && errno == EINTR) {
			goto RETRY_WRITE_MSG;
		}
		//just send one part , close connection
		_close_fd(stcp, fd, dctcp_close_reason_type::DCTCP_MSG_ERR);
	}
	//errno 
	return -2;
}

inline bool _is_listenner(dctcp_t * stcp, int fd){
	return	stcp->listeners.find(fd) != stcp->listeners.end();
}
inline void _add_listenner(dctcp_t * stcp, int fd, const sockaddr_in & addr){
	assert(stcp->listeners.find(fd) == stcp->listeners.end());
	dctcp_listener_t listener;
	listener.listenaddr = addr;
	stcp->listeners[fd] = listener;
}
static void _proc(dctcp_t * stcp,const epoll_event & ev){
	//schedule
	//error check
	if (ev.events & EPOLLRDHUP){
		//peer close
		_close_fd(stcp, ev.data.fd, dctcp_close_reason_type::DCTCP_PEER_CLOSE);
	}
	else if (ev.events & EPOLLERR){
		//error 
		_close_fd(stcp, ev.data.fd, dctcp_close_reason_type::DCTCP_POLL_ERR);
	}
	else if ((ev.events & EPOLLHUP)){
		//before listen/connect  add in poll ?
		_close_fd(stcp, ev.data.fd, dctcp_close_reason_type::DCTCP_INVAL_CALL);
	}
	else if (ev.events & EPOLLOUT){
		_connect_check(stcp, ev.data.fd);
	}
	else if (ev.events & EPOLLIN){
		if (_is_listenner(stcp, ev.data.fd)){
			//new connection
			_new_connx(stcp, ev.data.fd);
		}
		else{
			_read_tcp_socket(stcp, ev.data.fd);
		}
	}
	else {
		//error
		_close_fd(stcp, ev.data.fd, dctcp_close_reason_type::DCTCP_SYS_ERR);
	}
}
void			dctcp_close(dctcp_t * stcp, int fd){
	_close_fd(stcp, fd, dctcp_close_reason_type::DCTCP_CLOSE_ACTIVE);
}
int            dctcp_poll(dctcp_t * stcp, int timeout_us, int max_proc){
	PROFILE_FUNC();
	if (stcp->listeners.empty() &&
		stcp->connectings.empty()){
		return 0;
	}
	int ms = timeout_us / 1000;
	int nproc = 0;
	for (; stcp->nproc < stcp->nevts && nproc < max_proc; ++(stcp->nproc)){
		++nproc;
		_proc(stcp, stcp->events[stcp->nproc]);
	}
	if (stcp->nproc < stcp->nevts){
		//busy
		return nproc;
	}
	else{
		stcp->nproc = 0;
		stcp->nevts = 0;
	}
	int n = epoll_wait(stcp->epfd, stcp->events, stcp->conf.max_client, ms);
	for (int i = 0; i < n && nproc < max_proc; ++i){
		nproc++;
		_proc(stcp, stcp->events[i]);
	}
	return nproc;
}
int				dctcp_send(dctcp_t * stcp, int fd, const dctcp_msg_t & msg){
	if (fd < 0) {return -1; }
	return _write_tcp_socket(stcp, fd, msg.buff, msg.buff_sz);
}
int				dctcp_reconnect(dctcp_t* stcp, int fd){
	auto it = stcp->connectings.find(fd);
	dctcp_connecting_t * cnx = nullptr;
	if (it != stcp->connectings.end()) {
		cnx = &it->second;
	}
	else{
		return -1;
	}
	socklen_t addrlen = sizeof(cnx->connect_addr);
	int ret = connect(fd, (sockaddr*)&cnx->connect_addr, addrlen);
	if (ret && errno != EALREADY &&
			   errno != EINPROGRESS) {
		//error
		return -1;
	}
	cnx->reconnect++;
	GLOG_TRA("tcp connect fd:%d tried:%d  ....", fd, cnx->reconnect);
	return _op_poll(stcp, EPOLL_CTL_ADD, fd, EPOLLOUT);
}
int				dctcp_listen(dctcp_t * stcp, const dctcp_addr_t & addr){ //return a fd >= 0when success
	int fd = _create_tcpsocket(stcp->conf.max_tcp_send_buff_size, stcp->conf.max_tcp_recv_buff_size);
	if (fd < 0) { return -1; }
	sockaddr_in addrin;
	memset(&addrin, 0, sizeof(addrin));
	addrin.sin_family = AF_INET;
	addrin.sin_port = htons(addr.port);
	addrin.sin_addr.s_addr = addr.u32ip();
	int ret = bind(fd, (struct sockaddr *)&addrin, sizeof(struct sockaddr));
	if (ret) { close(fd); return -2; }
	ret = listen(fd, stcp->conf.max_backlog);
	if (ret) { close(fd); return -3; }

	epoll_event evt;
	evt.events = EPOLLIN | EPOLLET;
	evt.data.fd = fd;
	ret = epoll_ctl(stcp->epfd, EPOLL_CTL_ADD, fd, &evt);
	if (ret) { close(fd); return -4; };

	_add_listenner(stcp,fd, addrin);
	return fd;
}
int             dctcp_connect(dctcp_t * stcp, const dctcp_addr_t & addr, int retry){
	//allocate
	int fd = _create_tcpsocket(stcp->conf.max_tcp_send_buff_size, stcp->conf.max_tcp_recv_buff_size);
	if (fd < 0) return -1;//socket error
	int on = 1;
	_set_socket_opt(fd, SO_KEEPALIVE, &on, sizeof(on));

	sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = addr.u32ip();
	saddr.sin_port = htons(addr.port);
	dctcp_connecting_t cnx;
	cnx.max_reconnect = retry;
	cnx.reconnect = 0;
	cnx.connect_addr = saddr;
	stcp->connectings[fd] = cnx;

	return dctcp_reconnect(stcp, fd);
}
