#pragma once
#include "ext_meta.h"
#include <map>
struct st_mysql;
class MySQLMsgConverter;
struct MySQLMsgMeta {
	MySQLMsgConverter	*					cvt;
	const google::protobuf::Descriptor *	msg_desc;
	const google::protobuf::Message *		msg;
	EXTMessageMeta							meta;
	int32_t									table_idx;
public:
	MySQLMsgMeta(MySQLMsgConverter * pCvt);
	int		AttachMsg(const google::protobuf::Message *		msg);
private:
	void	construct();
public:
	std::string		GetTableName();
	int32_t			GetTableIdx();
	int				Select(std::string & sql, std::vector<std::string> * fields = nullptr, const char * where_ = nullptr);
	int				Delete(std::string & sql, const char * where_ = nullptr);
	int				Replace(std::string & sql);
	int				Update(std::string & sql);
	int				Insert(std::string & sql);
	int				CreateTable(std::string & sql);
	int				DropTable(std::string & sql);
};

struct st_mysql_field;
struct MySQLRow {
	st_mysql_field *    res_fields;
	int					num_fields;
	char **				row_data;
	unsigned long *		row_lengths;
};

class MySQLMsgConverter {
	std::string			meta_file;
	st_mysql *			mysql;
	std::string			field_buffer;
	std::string			escaped_buffer;
	EXTProtoMeta		protometa; //dynloading
public:
	MySQLMsgConverter(const std::string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER = 1024 * 1024);
public:
	std::string		GetFieldValue(const google::protobuf::Message & msg, const char * key);
	int				GetFieldsSQLKList(const google::protobuf::Message & msg, std::vector<std::pair<std::string, std::string> > & values);
	std::string		GetTableName(const char * msg_type, int idx = -1);
	std::string		GetMsgTypeNameFromTableName(const std::string & table_name);
	int				SetFieldValue(google::protobuf::Message & msg,const std::string & key,const char * value, size_t value_length);
public:
	int				InitSchema();
	int				CreateTables(const char * msg_type, std::string & sql, int idx = -1);
	int				CreateDB(const char * db_name, std::string & sql);
	int				DropDB(const char * db_name, std::string & sql);
	//todo
	int					AlterTables(std::map<std::string, std::string> old_fields_type, std::string & sql);
	EXTProtoMeta &		GetProtoMeta() { return protometa; }

	int					GetMsgBufferFromMySQLRow(char * buffer, int * buffer_len, const MySQLRow &  row);

};
