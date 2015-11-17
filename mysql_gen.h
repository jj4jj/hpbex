#pragma once
#include "ext_meta.h"
struct st_mysql;

class MySQLMsgConverter {
	std::string			meta_file;
	st_mysql *			mysql;
	const google::protobuf::Descriptor *	desc;
	const google::protobuf::Message *	msg;
	std::string				msg_name;
	STMessageMeta			meta;
	std::string			field_buffer;
	std::string			escaped_buffer;
	ProtoMeta			pm;
public:
	MySQLMsgConverter(const std::string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER = 1024 * 1024);
private:
	std::string	GetFieldValue(const char * key);
	int		GetFieldsSQLKList(std::vector<std::pair<std::string, std::string> > & values);
public:
	std::string GetTableName();
	int		InitSchema(const google::protobuf::Message & msg);
	int		Select(std::string & sql, std::vector<std::string> * fields = nullptr, const char * where_ = nullptr);
	int		Delete(std::string & sql, const char * where_ = nullptr);
	int		Replace(std::string & sql);
	int		Update(std::string & sql);
	int		Insert(std::string & sql);
};



//message -> mysql
//update
//insert
//delete
//replace
//select (custom)

//create
//alter
//drop


