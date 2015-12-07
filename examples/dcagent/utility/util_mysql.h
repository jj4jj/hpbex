#pragma  once
#include "base/stdinc.h"

NS_BEGIN(dcsutil)

struct mysqlclient_t {
	struct table_row_t {
		size_t			fields_count;
		const char * *	fields_name;
		const char * *  row_data;
		size_t	*		row_length;
		//--------------------------
		size_t			row_total;
		size_t			row_offset;
		const char *	table_name;
	};
	struct cnnx_conf_t {
		std::string  ip;
		std::string  uname;
		std::string  passwd;
		std::string  dbname;
		std::string  unisock;
		std::string	 char_set;
		bool	auto_commit;
		bool	auto_reconnect;
		int     port;
		int     wait_timeout;
		int     intr_timeout;
		int		ping_chkgap;
		long    cliflag;
		cnnx_conf_t()
		{
			port = cliflag = wait_timeout = intr_timeout = 0;
			ip = uname = passwd = dbname = unisock = "";
			auto_commit = auto_reconnect = true;
			char_set = "utf8";
			ping_chkgap = 10;//10s
		}
	};
private:
	void	*	handle;
	//////////////////////////////////
public:
	mysqlclient_t();
	~mysqlclient_t();
	int			init(const cnnx_conf_t & conf);
	int			execute(const std::string & sql);
	size_t		affects();
	int			commit();//if not auto commit
	int			ping(); //return 1:reconnected, 0:status ok; -1: ping error
	//return 1:fetch next, return 0:
	typedef void	result_cb_func_t(void* ud, OUT bool & need_more, const table_row_t & row);
	int				result(void * ud, result_cb_func_t cb);//get result for select
	//error sth 
	int				err_no();
	const char *	err_msg();
	//mysql conn
	void *			mysql_handle();
};

NS_END()
