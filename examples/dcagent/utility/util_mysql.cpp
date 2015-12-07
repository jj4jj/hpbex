#include "base/logger.h"
#include "util_mysql.h"
extern "C" {
	#include "mysql/mysql.h"
}
	
NS_BEGIN(dcsutil)

///////////////////////////////////////////////////////////////
static const int MAX_MYSQL_ERR_MSG_SZ = 1024;
struct mysqlclient_impl_t {
	MYSQL *			mysql_conn;
	///////////////////////////////////////////////////////////
	mysqlclient_t::cnnx_conf_t		conf;
	std::string		error_msg;
	time_t			last_ping_time;
	mysqlclient_impl_t() {
		mysql_conn = NULL;
		last_ping_time = 0;
		error_msg.reserve(MAX_MYSQL_ERR_MSG_SZ);
	}
};

#define _THIS_HANDLE	((mysqlclient_impl_t*)(handle))
#define LOG_S(format, ...)	LOGSTR(_THIS_HANDLE->error_msg, "mysql", " [%d (%s)]" format,mysql_errno(_THIS_HANDLE->mysql_conn),mysql_error(_THIS_HANDLE->mysql_conn), ##__VA_ARGS__)

mysqlclient_t::mysqlclient_t(){
	handle = new mysqlclient_impl_t();
}

inline	void	mysqlclient_cleanup(void *handle){
	if (_THIS_HANDLE){
		if (_THIS_HANDLE->mysql_conn){
			mysql_close(_THIS_HANDLE->mysql_conn);
			_THIS_HANDLE->mysql_conn = NULL;
		}
	}
}

mysqlclient_t::~mysqlclient_t(){
	mysqlclient_cleanup(handle);
	if(_THIS_HANDLE)
		delete _THIS_HANDLE;
}
//
int		mysqlclient_t::init(const mysqlclient_t::cnnx_conf_t & conf){
	mysqlclient_cleanup(handle); //for reinit
	auto conn = mysql_init(NULL);
	if (!conn){
		LOG_S("mysql client init error ");
		return -1;
	}
	char tmpset[255] = "";
	if (conf.wait_timeout > 0){
		sprintf(tmpset, "set wait_timeout=%d", conf.wait_timeout);
		mysql_options(conn, MYSQL_INIT_COMMAND, tmpset);
		mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &conf.wait_timeout);
		mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &conf.wait_timeout);
		mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &conf.wait_timeout);
	}
	if (conf.intr_timeout > 0)
	{
		sprintf(tmpset, "set interactive_timeout=%d", conf.intr_timeout);
		mysql_options(conn, MYSQL_INIT_COMMAND, tmpset);
	}
	//auto reconnect
	bool auto_reconnect = conf.auto_reconnect ? 1 : 0;
	mysql_options(conn, MYSQL_OPT_RECONNECT, &auto_reconnect);

	//connect to server
	if (!mysql_real_connect(conn, conf.ip.c_str(), conf.uname.c_str(),
		conf.passwd.c_str(), NULL, conf.port,
		conf.unisock.c_str(), conf.cliflag)){
		LOG_S("mysql_real_connectn error ");
		goto FAIL_CONN;
	}

	if (mysql_set_character_set(conn, conf.char_set.c_str())){
		LOG_S("set charset error ");
		goto FAIL_CONN;
	}
	if (mysql_autocommit(conn, conf.auto_commit ? 1 : 0)){
		LOG_S("auto commit set error ");
		goto FAIL_CONN;
	}
	////////////////////////////////////////////////
	_THIS_HANDLE->conf = conf;
	_THIS_HANDLE->mysql_conn = conn;
	return 0;
FAIL_CONN:
	mysql_close(conn);
	return -2;
}
size_t	mysqlclient_t::affects(){
	return mysql_affected_rows(_THIS_HANDLE->mysql_conn);
}

//return affects num
int		mysqlclient_t::execute(const std::string & sql){
	GLOG_TRA("exec sql = \n[%s]\n", sql.c_str());
	if (mysql_query(_THIS_HANDLE->mysql_conn, sql.c_str())){
		LOG_S("execute sql:%s error ", sql.c_str());
		return -1;
	}
	return 0;
}
int		mysqlclient_t::commit(){//if not auto commit
	if (mysql_commit(_THIS_HANDLE->mysql_conn)){
		LOG_S("commit error ");
	}
	return 0;
}
int		mysqlclient_t::ping(){
	time_t tnow = time(NULL);
	if (tnow < _THIS_HANDLE->last_ping_time + _THIS_HANDLE->conf.ping_chkgap)	
		return 0;
	_THIS_HANDLE->last_ping_time = tnow;
	unsigned long mtid1 = mysql_thread_id(_THIS_HANDLE->mysql_conn);
	int ret = mysql_ping(_THIS_HANDLE->mysql_conn);
	if (ret){
		LOG_S("mysql server ping error !");
		return -1;
	}
	unsigned long mtid2 = mysql_thread_id(_THIS_HANDLE->mysql_conn);
	if (mtid2 != mtid1)	return 1;
	return 0;
}
int		mysqlclient_t::result(void * ud, result_cb_func_t cb){//get result for select
	MYSQL_RES *res_set = mysql_store_result(_THIS_HANDLE->mysql_conn);
	if (res_set == NULL){
		LOG_S("mysql_store_result failed ");
		return -1;
	}
	struct table_row_t row_store;
	row_store.row_offset = 0;
	row_store.row_total = mysql_num_rows(res_set);
	row_store.fields_count = mysql_field_count(_THIS_HANDLE->mysql_conn);
	bool	need_more = true;
	int		ret = 0;
	string table_name = "";
	MYSQL_FIELD * fields_all = mysql_fetch_fields(res_set);
	if (row_store.row_total == 0 || row_store.fields_count == 0){
		goto FREE_RESULT;
	}
	row_store.fields_name = (const char **)malloc(sizeof(char*) * row_store.fields_count);
	if (!row_store.fields_name){
		ret = -2;
		LOG_S("malloc row store field error field count:%zu row total:%zu!",
			row_store.fields_count, row_store.row_total);
		goto FREE_RESULT;
	}
	for (size_t i = 0; i < row_store.fields_count; ++i){
		row_store.fields_name[i] = fields_all[i].name;
		if (table_name.empty()){
			table_name = fields_all[i].table;
			row_store.table_name = table_name.c_str();
		}
	}
	for (; row_store.row_offset < row_store.row_total && need_more; ++row_store.row_offset){
		row_store.row_data = (const char **)mysql_fetch_row(res_set);
		row_store.row_length = mysql_fetch_lengths(res_set);
		need_more = (row_store.row_offset + 1) < row_store.row_total;
		cb(ud, need_more, row_store);
	}
	ret = (int)row_store.row_offset;
FREE_RESULT:
	free(row_store.fields_name);
	mysql_free_result(res_set);
	return ret;
}
int				mysqlclient_t::err_no(){
	return mysql_errno(_THIS_HANDLE->mysql_conn);
}
const char *	mysqlclient_t::err_msg(){
	return _THIS_HANDLE->error_msg.c_str();
}
void *			mysqlclient_t::mysql_handle(){
	return _THIS_HANDLE->mysql_conn;
}




NS_END()