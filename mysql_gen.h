#pragma once
#include "ext_meta.h"
#include <map>
struct st_mysql;
class MySQLMsgCvt;
struct MySQLMsgMeta {
	MySQLMsgCvt	*					cvt;
	const google::protobuf::Descriptor *	msg_desc;
	const google::protobuf::Message *		msg;
	EXTMessageMeta							meta;
	int32_t									table_idx;
public:
	MySQLMsgMeta(MySQLMsgCvt * pCvt);
	int		AttachMsg(const google::protobuf::Message *		msg, bool flatmode = false);
private:
	void	construct();
public:
	std::string		GetTableName();
	int32_t			GetTableIdx();
	int				Select(std::string & sql, std::vector<std::string> * fields = nullptr, const char * where_ = nullptr,
							int offset = 0, int limit = 0, const char * orderby = nullptr, int order = 1, bool flatmode = false);
	int				Delete(std::string & sql, const char * where_ = nullptr, bool flatmode = false);
	int				Replace(std::string & sql , bool flatmode = false);
	int				Update(std::string & sql , bool flatmode = false);
	int				Insert(std::string & sql , bool flatmode = false);
	int				CreateTable(std::string & sql, bool flatmode = false);
	int				DropTable(std::string & sql);
    int             Count(std::string & sql, const char * where_ = nullptr);

};

struct st_mysql_field;
struct MySQLRow {
	const char *				table_name;
	const char * *				fields_name;
	int							num_fields;
	const char **				row_data;
	unsigned long *				row_lengths;
};


class MySQLMsgCvt {
	std::string			meta_file;
	void *			    mysql;
	std::string			field_buffer;
	std::string			escaped_buffer;
    std::string         package_name;
	EXTProtoMeta		protometa; //dynloading
public:
	MySQLMsgCvt(const std::string & file, void * mysqlconn, size_t MAX_FIELD_BUFFER = 1024 * 1024);

public:
    int					InitMeta(int n = 0, const char ** path = nullptr, int m = 0, const char ** otherfiles = nullptr);
	int					CheckMsgValid(const google::protobuf::Descriptor * msg_desc, bool root = true, bool flatmode = false);
	int					CreateTables(const char * msg_type, std::string & sql, int idx = -1);
	int					CreateFlatTables(const char * msg_type, std::string & sql, int idx = -1);

	int					CreateDB(const char * db_name, std::string & sql);
	int					DropDB(const char * db_name, std::string & sql);
	//todo
	int					AlterTables(std::map<std::string, std::string> old_fields_type, std::string & sql);
	EXTProtoMeta &		GetProtoMeta() { return protometa; }

	//top layer unfold
	int					GetMsgBufferFromSQLRow(char * buffer, int * buffer_len, const MySQLRow &  row, bool faltmode = false);
    int					GetMsgFromSQLRow(google::protobuf::Message & msg, const MySQLRow &  row, bool faltmode = false);
    int                 escape_string(std::string & result, const char * data, int datalen);
    //---------------------------------------------------------------------------------------------
public:
	//for msg mysql meta
	std::string		GetMsgFieldValue(const google::protobuf::Message & msg, const char * key);
	//top layer unfold msg
	int				GetMsgSQLKVList(const google::protobuf::Message & msg,
		std::vector<std::pair<std::string, std::string> > & values,
		bool readmode = true);

	//unfload all to basic mysql type (no blob)
	int				GetMsgSQLFlatKVList(const google::protobuf::Message & msg,
		std::vector<std::pair<std::string, std::string> > & values,
		const std::string & prefix);

	std::string		GetTableName(const char * msg_type, int idx = -1);

private:
	static	std::string		GetRepeatedFieldLengthName(const std::string & name);
	static	std::string		GetRepeatedFieldName(const std::string & name, int idx);
	static  bool			IsRepeatedFieldLength(const std::string & field_name,
		const std::string & key);
	static	int				GetRepeatedFieldIdx(const std::string & field_name,
		const std::string & key);

private:
	int				GetMsgSQLFlatKVRepeated(const google::protobuf::Message & msg,
		const google::protobuf::Reflection * pReflection,
		const google::protobuf::FieldDescriptor * pField, int idx,
		std::vector<std::pair<std::string, std::string> > & values,
		const std::string & prefix);

	int				SetMsgSQLFlatKVList(google::protobuf::Message & msg, const MySQLRow &  row);
	int				SetMsgSQLFlatKV(google::protobuf::Message & msg, const std::string & key,
		const char * value, size_t value_length);
	int				SetMsgSQLFlatKVRepeated(google::protobuf::Message & msg,
		const google::protobuf::Reflection * pReflection,
		const google::protobuf::FieldDescriptor * pField, int idx,
		const std::string & key, const char * value, size_t value_length);
	//---------------------------------------------------------------------
	int				GetMsgSQLKV(const google::protobuf::Message & msg,
		const google::protobuf::FieldDescriptor * pField,
		std::pair<std::string, std::string> & kv);

	std::string		GetMsgTypeNameFromTableName(const std::string & table_name);

	int				SetMsgFieldMySQLValue(google::protobuf::Message & msg, const std::string & key,
		const char * value, size_t value_length);

	int				GetMsgFlatTableSQLFields(const google::protobuf::Descriptor * msg_desc,
		std::string & sql,
		const std::string & prefix);
};
