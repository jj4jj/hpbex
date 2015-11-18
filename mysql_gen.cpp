#include "mysql_gen.h"
#include "mysql/mysql.h"
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;




MySQLMsgMeta::MySQLMsgMeta(MySQLMsgConverter * pCvt):
			cvt(pCvt),msg_desc(nullptr),msg(nullptr){
}
void	MySQLMsgMeta::construct(){
	msg_desc = nullptr;
	msg = nullptr;
}

int	MySQLMsgMeta::AttachMsg(const google::protobuf::Message *	msg_){
	construct();
	meta.construct();
	//////////////////////////////////////////////////
	if (!msg_){
		error_stream << "error msg is null !" << endl;
		return -1;
	}
	msg = msg_;
	msg_desc = msg->GetDescriptor();
	///////////////////////////////////////////////
	if (!msg_desc){
		return -2;
	}
	if (meta.AttachDesc(msg_desc)){
		//error parse from desc
		error_stream << "error parse from desc " << endl;
		return -3;
	}
	if (meta.pks_name.empty()){
		//not found the pk def
		error_stream << "not found the pk def " << endl;
		return -4;
	}
	if (meta.pks_name.size() != meta.pks_fields.size()){
		error_stream << "meta pks size not match " << endl;
		return -5;
	}
	//check field type
	for (auto & sfm : meta.sub_fields){
		if (sfm.field_desc->is_repeated()){
			error_stream << "not support db field proto repeated ! error field :" << sfm.field_desc->name() << endl;
			return -6;
		}
	}
	if (meta.m_divnum < 0){
		//must be a integer 
		error_stream << "meta divnum is error :" << meta.m_divnum << endl;
		return -7;
	}
	return 0;
}
int32_t			MySQLMsgMeta::GetTableIdx(){
	if (meta.m_divnum <= 0){
		return -1;
	}	
	uint64_t ullspkey = std::stoull(cvt->GetFieldValue(*msg, meta.m_divkey.c_str()));
	return ullspkey % meta.m_divnum;
}
std::string		MySQLMsgMeta::GetTableName(){
	string tb_name = msg_desc->name();
	int idx = GetTableIdx();
	if (idx >= 0){
		tb_name += "_";
		tb_name += to_string(idx);
	}
	return tb_name;
}
int				MySQLMsgMeta::Select(std::string & sql, std::vector<std::string> * fields, const char * where_){
	//
	sql = "SELECT ";
	if (fields){
		for (int i = 0; i < (int)fields->size(); ++i){
			if (i != 0){
				sql += ",";
			}
			sql += fields->at(i);
		}
	}
	else {
		for (int i = 0; i < (int)msg_desc->field_count(); ++i){
			if (i != 0){
				sql += ",";
			}
			sql += msg_desc->field(i)->name();
		}
	}
	sql += " FROM `";
	sql += GetTableName();
	sql += "` WHERE ";
	if (where_){
		sql += where_;
	}
	else {
		int is_first = 0;
		int is_pk = 0;
		std::vector<std::pair<std::string, std::string > > kvlist;
		int ret = cvt->GetFieldsSQLKList(*msg, kvlist);
		if (ret) return ret;
		for (auto & kv : kvlist)
		{
			is_pk = 0;
			for (auto & pk : meta.pks_name)
			{
				if (kv.first == pk)
				{
					is_pk = true;
					break;
				}
			}
			if (!is_pk)
			{
				continue;
			}
			if (is_first != 0)
			{
				sql += " AND ";
			}
			is_first = 1;

			sql += kv.first;
			sql += "=";
			sql += kv.second;
		}
	}
	return 0;
}
int		MySQLMsgMeta::Delete(std::string & sql, const char * where_){
	//
	sql = "DELETE ";
	sql += " FROM `";
	sql += GetTableName();
	sql += "` WHERE ";
	if (where_){
		sql += where_;
	}
	else {
		int is_first = 0;
		int is_pk = 0;
		std::vector<std::pair<std::string, std::string > > kvlist;
		int ret = cvt->GetFieldsSQLKList(*msg, kvlist);
		if (ret) return ret;
		for (auto & kv : kvlist)
		{
			is_pk = 0;
			for (auto & pk : meta.pks_name)
			{
				if (kv.first == pk)
				{
					is_pk = true;
					break;
				}
			}
			if (!is_pk)
			{
				continue;
			}
			if (is_first != 0)
			{
				sql += " AND ";
			}
			is_first = 1;

			sql += kv.first;
			sql += "=";
			sql += kv.second;
		}
	}
	return 0;
}
int		MySQLMsgMeta::Replace(std::string & sql){
	//REPLACE INTO 
	sql = "REPLACE INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = cvt->GetFieldsSQLKList(*msg, kvlist);
	if (ret) return ret;
	for (int i = 0; i < (int)kvlist.size(); ++i)
	{
		if (i != 0)
		{
			sql += ",";
		}
		sql += kvlist[i].first;
	}
	sql += ") VALUES (";
	//VALUES(vs)
	for (int i = 0; i < (int)kvlist.size(); ++i)
	{
		if (0 != i)
		{
			sql += ",";
		}
		sql += kvlist[i].second;
	}
	sql += ")";
	return 0;
}
int				MySQLMsgMeta::Update(std::string & sql){
	/*
	MySQL
	UPDATE[LOW_PRIORITY][IGNORE] tbl_name
	SET col_name1 = expr1[, col_name2 = expr2 ...]
	[WHERE where_definition]
	[ORDER BY ...]
	[LIMIT row_count]
	*/
	sql = "UPDATE `";
	sql += GetTableName();
	sql += "` SET ";
	std::vector<std::pair<string, string> > kvlist;
	int ret = cvt->GetFieldsSQLKList(*msg, kvlist);
	if (ret) return ret;
	int is_first = 0;
	int is_pk = 0;
	for (auto & kv : kvlist)
	{
		is_pk = 0;
		for (int j = 0; j < (int)meta.pks_name.size(); ++j)
		{
			if (kv.first == meta.pks_name[j])
			{
				is_pk = true;
				break;
			}
		}
		if (is_pk || kv.first == meta.m_autoinc)
		{
			continue;
		}
		if (is_first != 0)
		{
			sql += " , ";
		}
		is_first = 1;

		sql += kv.first;
		sql += "=";
		sql += kv.second;
	}
	if (!meta.m_autoinc.empty())
	{
		if (is_first != 0)
		{
			sql += " , ";
		}
		sql += meta.m_autoinc;
		sql += "=";
		sql += meta.m_autoinc;
		sql += "+1";
	}
	//where
	sql += " WHERE ";
	is_first = 0;
	for (auto & kv : kvlist)
	{
		is_pk = 0;
		for (auto & pk : meta.pks_name)
		{
			if (kv.first == pk)
			{
				is_pk = true;
				break;
			}
		}
		if (!is_pk)
		{
			continue;
		}
		if (is_first != 0)
		{
			sql += " AND ";
		}
		is_first = 1;

		sql += kv.first;
		sql += "=";
		sql += kv.second;
	}
	return 0;
}
int				MySQLMsgMeta::Insert(std::string & sql){
	//INSERT INTO order_(...)	values
	//update order_()	set		where
	sql = "INSERT INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = cvt->GetFieldsSQLKList(*msg, kvlist);
	if (ret) return ret;
	for (int i = 0; i < (int)kvlist.size(); ++i)
	{
		if (i != 0)
		{
			sql += ",";
		}
		sql += kvlist[i].first;
	}
	if (!meta.m_autoinc.empty())
	{
		sql += ",";
		sql += meta.m_autoinc;
	}
	sql += ") VALUES (";
	//VALUES(vs)
	for (int i = 0; i < (int)kvlist.size(); ++i)
	{
		if (0 != i)
		{
			sql += ",";
		}
		sql += kvlist[i].second;
	}
	if (!meta.m_autoinc.empty())
	{
		sql += ",";
		sql += "0";
	}
	sql += ")";
	return 0;
}
int			MySQLMsgMeta::DropTable(std::string & sql){
	sql = "DROP TABLE IF EXISTS ";
	sql += GetTableName();
	return 0;
}

int			MySQLMsgMeta::CreateTable(std::string & sql){
	return cvt->CreateTables(msg_desc->name().c_str(), sql, GetTableIdx());
}

///////////////////////////////////////////////////////////////////////////////////////////

MySQLMsgConverter::MySQLMsgConverter(const string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER) :meta_file(file), mysql(pMysql){
	field_buffer.reserve(MAX_FIELD_BUFFER);//1MB
	escaped_buffer.reserve(MAX_FIELD_BUFFER * 2 + 1);
}
string	MySQLMsgConverter::GetFieldValue(const google::protobuf::Message & msg, const char * key){
	const Reflection * pReflection = msg.GetReflection();
	auto msg_desc = msg.GetDescriptor();
	for (int i = 0; i < msg_desc->field_count(); ++i){
		const FieldDescriptor * pField = msg_desc->field(i);
		if (!pField || pField->name() != key){
			continue;
		}
		switch (pField->cpp_type())
		{
		case FieldDescriptor::CPPTYPE_FLOAT:
			return std::to_string(pReflection->GetFloat(msg, pField));
		case FieldDescriptor::CPPTYPE_DOUBLE:
			return std::to_string(pReflection->GetDouble(msg, pField));
		case FieldDescriptor::CPPTYPE_INT32:
			return std::to_string(pReflection->GetInt32(msg, pField));
		case FieldDescriptor::CPPTYPE_INT64:
			return std::to_string(pReflection->GetInt64(msg, pField));
		case FieldDescriptor::CPPTYPE_UINT32:
			return std::to_string(pReflection->GetUInt32(msg, pField));
		case FieldDescriptor::CPPTYPE_UINT64:
			return std::to_string(pReflection->GetUInt64(msg, pField));
		case FieldDescriptor::CPPTYPE_ENUM:
			return std::to_string(pReflection->GetEnum(msg, pField)->number());
		case FieldDescriptor::CPPTYPE_BOOL:
			return std::to_string(pReflection->GetBool(msg, pField));
		case FieldDescriptor::CPPTYPE_STRING:
			return pReflection->GetString(msg, pField);
		case FieldDescriptor::CPPTYPE_MESSAGE:
			return "";
		default:
			return "";
		}
	}
	return "";
}
int		MySQLMsgConverter::GetFieldsSQLKList(const google::protobuf::Message & msg, std::vector<std::pair<string, string> > & values)
{
	values.clear();
	const Reflection * pReflection = msg.GetReflection();
	auto msg_desc = msg.GetDescriptor();
	for (int i = 0; i < msg_desc->field_count(); ++i)
	{
		const FieldDescriptor * pField = msg_desc->field(i);
		if (!pField){
			continue;
		}
		std::pair<std::string, std::string> kv;
		kv.first = pField->name();
		kv.second = "";
		field_buffer = "";
		char * buffer_msg = (char*)field_buffer.data();
		size_t buffer_len = 0;
		switch (pField->cpp_type())
		{
		case FieldDescriptor::CPPTYPE_FLOAT:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetFloat(msg, pField));
			}
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetDouble(msg, pField)); \
			}
			break;
		case FieldDescriptor::CPPTYPE_INT32:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetInt32(msg, pField));
			}
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetInt64(msg, pField));
			}
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetUInt32(msg, pField));
			}
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetUInt64(msg, pField));
			}
			break;
		case FieldDescriptor::CPPTYPE_ENUM:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetEnum(msg, pField)->number());
			}
			break;
		case FieldDescriptor::CPPTYPE_BOOL:
			if (pReflection->HasField(msg, pField)){
				field_buffer = std::to_string(pReflection->GetBool(msg, pField));
			}
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			if (pReflection->HasField(msg, pField)){
				field_buffer.assign(pReflection->GetString(msg, pField));
				if (field_buffer.empty()){
					field_buffer = "''";
				}
			}
			break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
			if (pReflection->HasField(msg, pField)){
				const Message & _tmpmsg = pReflection->GetMessage(msg, pField);
				_tmpmsg.SerializeToArray((char*)field_buffer.data(), field_buffer.capacity());
				buffer_msg = (char*)field_buffer.data();
				buffer_len = _tmpmsg.ByteSize();
			}
			break;
		default:
			return -1;
		}
		if (buffer_len == 0){
			if (field_buffer.empty()){
				if (pField->cpp_type() <= FieldDescriptor::CPPTYPE_ENUM){
					field_buffer = "0";
					buffer_len = 1;
				}
				else { // if (pField->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) 
					field_buffer = "''";
					buffer_len = 2;
				}
			}
			else{
				buffer_len = field_buffer.length();
			}
		}
		//need escape
		if (mysql){
			bzero((char*)escaped_buffer.data() + buffer_len,
				buffer_len + 1); //
			mysql_real_escape_string(mysql, (char*)&escaped_buffer[1], buffer_msg, buffer_len);
			kv.second = escaped_buffer.data();
		}
		else {
			kv.second = buffer_msg;
		}

		values.push_back(kv);
	}
	return 0;
}
int		MySQLMsgConverter::InitSchema(){
	if (protometa.Init(nullptr)){
		error_stream << "proto meta init error !" << endl;
		return -1;
	}
	if (!protometa.LoadFile(meta_file.c_str())){
		error_stream << "proto meta load error !" << endl;
		return -2;
	}
	return 0;
}
string		MySQLMsgConverter::GetTableName(const char * msg_type, int idx){
	string tbname = msg_type;
	if (idx >= 0){
		tbname += "_";
		tbname += to_string(idx);
	}
	return tbname;
}
int			MySQLMsgConverter::CreateTables(const char * msg_type, std::string & sql,int idx ){
	auto msg_desc = GetMsgDesc(msg_type);
	if (!msg_desc){
		return -1;
	}
	EXTMessageMeta	meta;
	if (meta.AttachDesc(msg_desc)){
		return -2;
	}
	//template
	string table_name = GetTableName(msg_type, -1);
	sql = "CREATE TABLE IF NOT EXISTS `" + table_name + "` (";
	for (auto & field : meta.sub_fields)
	{
		sql += field.field_desc->name() + " " + field.GetMysqlFieldType();
		sql += " NOT NULL";
		sql += ",\n";
	}
	string pks = "";
	for (auto & pk : meta.pks_name)
	{
		if (!pks.empty()){
			pks += ",";
		}
		pks += pk;
	}
	sql += "PRIMARY KEY (";
	sql += pks + ")\n";
	sql += ") ENGINE=InnoDB DEFAULT CHARSET utf8 COLLATE utf8_general_ci;";
	string sql_real = "";
	if (idx < 0){
		for (int i = 0; i < meta.m_divnum; ++i){
			if (i != 0){
				sql_real += "\n";
			}
			auto new_table_name = GetTableName(msg_type, i);
			sql_real += sql.replace(sql.find(table_name), table_name.length(), new_table_name);
			table_name = new_table_name;
		}
		sql = sql_real;
	}
	return 0;
}

int			MySQLMsgConverter::CreateDB(const char * db_name, std::string & sql){
	sql = "CREATE DATABASE IF NOT EXISTS ";
	sql += db_name;
	sql += " DEFAULT CHARSET utf8 COLLATE utf8_general_ci";
	return 0;
}
int			MySQLMsgConverter::DropDB(const char * db_name, std::string & sql){
	sql = "DROP DATABASE IF EXISTS ";
	sql += db_name;
	return 0;
}


const Descriptor * MySQLMsgConverter::GetMsgDesc(const char * msg_type){
	return protometa.GetPool()->FindMessageTypeByName(msg_type);
}
