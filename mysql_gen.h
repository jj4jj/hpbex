#pragma once
#include "ext_meta.h"
struct st_mysql;

struct MySQLMsgMeta {
	const google::protobuf::Descriptor *	msg_desc;
	const google::protobuf::Message *		msg;
	MySQLMsgConverter	*					cvt;
	EXTMessageMeta							meta;
public:
	MySQLMsgMeta(MySQLMsgConverter * pCvt,
		const google::protobuf::Descriptor * msg_desc_ = nullptr,
		const google::protobuf::Message * msg_ = nullptr);
	int		Init();
public:
	std::string		GetTableName();
	int				Select(std::string & sql, std::vector<std::string> * fields = nullptr, const char * where_ = nullptr);
	int				Delete(std::string & sql, const char * where_ = nullptr);
	int				Replace(std::string & sql);
	int				Update(std::string & sql);
	int				Insert(std::string & sql);
	int				CreateTable(std::string & sql);
	int				AlterTable(std::map<std::string, std::string> old_fields_type,std::string & sql);
	int				DropTable(std::string & sql);
};

class MySQLMsgConverter {
	std::string			meta_file;
	st_mysql *			mysql;
	std::string			field_buffer;
	std::string			escaped_buffer;
	ProtoMeta			protometa; //dynloading
public:
	MySQLMsgConverter(const std::string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER = 1024 * 1024);
public:
	std::string		GetFieldValue(const google::protobuf::Message & msg, const char * key);
	int				GetFieldsSQLKList(const google::protobuf::Message & msg, std::vector<std::pair<std::string, std::string> > & values);
public:
	int				InitSchema();
	MySQLMsgMeta		GetMySQLMsg(const char * msg_type, const google::protobuf::Message * msg);
	int				CreateTable(const char * msg_type, std::string & sql);
	int				CreateDB(const char * db_name, std::string & sql);
	int				DropDB(const char * db_name, std::string & sql);
};
