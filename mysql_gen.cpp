#include "mysql_gen.h"
#include "mysql/mysql.h"
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;

MySQLMsgConverter::MySQLMsgConverter(const string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER) :meta_file(file), mysql(pMysql){
	field_buffer.reserve(MAX_FIELD_BUFFER);//1MB
	escaped_buffer.reserve(MAX_FIELD_BUFFER * 2 + 1);
}
string	MySQLMsgConverter::GetFieldValue(const char * key){
	const Reflection * pReflection = msg->GetReflection();
	for (int i = 0; i < desc->field_count(); ++i){
		const FieldDescriptor * pField = desc->field(i);
		if (!pField || pField->name() != key){
			continue;
		}
		switch (pField->cpp_type())
		{
		case FieldDescriptor::CPPTYPE_FLOAT:
			return std::to_string(pReflection->GetFloat(*msg, pField));
		case FieldDescriptor::CPPTYPE_DOUBLE:
			return std::to_string(pReflection->GetDouble(*msg, pField));
		case FieldDescriptor::CPPTYPE_INT32:
			return std::to_string(pReflection->GetInt32(*msg, pField));
		case FieldDescriptor::CPPTYPE_INT64:
			return std::to_string(pReflection->GetInt64(*msg, pField));
		case FieldDescriptor::CPPTYPE_UINT32:
			return std::to_string(pReflection->GetUInt32(*msg, pField));
		case FieldDescriptor::CPPTYPE_UINT64:
			return std::to_string(pReflection->GetUInt64(*msg, pField));
		case FieldDescriptor::CPPTYPE_ENUM:
			return std::to_string(pReflection->GetEnum(*msg, pField)->number());
		case FieldDescriptor::CPPTYPE_BOOL:
			return std::to_string(pReflection->GetBool(*msg, pField));
		case FieldDescriptor::CPPTYPE_STRING:
			return pReflection->GetString(*msg, pField);
		case FieldDescriptor::CPPTYPE_MESSAGE:
			return "";
		default:
			return "";
		}
	}
	return "";
}
int		MySQLMsgConverter::GetFieldsSQLKList(std::vector<std::pair<string, string> > & values)
{
	values.clear();
	const Reflection * pReflection = msg->GetReflection();
	for (int i = 0; i < desc->field_count(); ++i)
	{
		const FieldDescriptor * pField = desc->field(i);
		if (!pField)
		{
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
			field_buffer = std::to_string(pReflection->GetFloat(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			field_buffer = std::to_string(pReflection->GetDouble(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_INT32:
			field_buffer = std::to_string(pReflection->GetInt32(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			field_buffer = std::to_string(pReflection->GetInt64(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			field_buffer = std::to_string(pReflection->GetUInt32(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			field_buffer = std::to_string(pReflection->GetUInt64(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_ENUM:
			field_buffer = std::to_string(pReflection->GetEnum(*msg, pField)->number());
			break;
		case FieldDescriptor::CPPTYPE_BOOL:
			field_buffer = std::to_string(pReflection->GetBool(*msg, pField));
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			field_buffer.assign(pReflection->GetString(*msg, pField));
			if (field_buffer.empty()){
				field_buffer = "''";
			}
			break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
		{
													const Message & _tmpmsg = pReflection->GetMessage(*msg, pField);
													_tmpmsg.SerializeToArray((char*)field_buffer.data(), field_buffer.capacity());
													buffer_msg = (char*)field_buffer.data();
													buffer_len = _tmpmsg.ByteSize();
		}
			break;
		default:
			return -1;
		}
		if (buffer_len == 0){
			buffer_len = field_buffer.length();
		}
		//need escape
		bzero((char*)escaped_buffer.data() + buffer_len,
			buffer_len + 1); //
		mysql_real_escape_string(mysql, (char*)&escaped_buffer[1], buffer_msg, buffer_len);
		kv.second = escaped_buffer.data();

		values.push_back(kv);
	}
	return 0;
}
string MySQLMsgConverter::GetTableName(){
	string tb_name = desc->name();
	if (!meta.m_divnum.empty()){
		uint64_t ullspkey = std::stoull(GetFieldValue(meta.m_divkey.c_str()));
		size_t spnum = std::stoi(meta.m_divnum);
		tb_name += "_" + std::to_string(ullspkey % spnum);
	}
	return tb_name;
}
int		MySQLMsgConverter::InitSchema(const google::protobuf::Message & msg_){
	msg = &msg_;
	msg_name = msg_.GetDescriptor()->name();
	if (protometa.Init(nullptr)){
		error_stream << "proto meta init error !" << endl;
		return -1;
	}
	if (!protometa.LoadFile(meta_file.c_str())){
		error_stream << "proto meta load error !" << endl;
		return -2;
	}
	desc = protometa.GetPool()->FindMessageTypeByName(msg_name.c_str());
	if (!desc){
		error_stream << "not found message type:" << msg_name << endl;
		return -3;
	}
	if (meta.ParseFrom(desc)){
		//error parse from desc
		error_stream << "error parse from desc " << endl;
		return -4;
	}
	if (meta.pks_name.empty()){
		//not found the pk def
		error_stream << "not found the pk def " << endl;
		return -5;
	}
	if (meta.pks_name.size() != meta.pks_fields.size()){
		error_stream << "meta pks size not match " << endl;
		return -6;
	}
	if (!meta.m_divnum.empty()){
		//must be a integer 
		if (std::stoi(meta.m_divnum) <= 0){
			error_stream << "meta divnum is error :" << meta.m_divnum << endl;
			return -7;
		}
	}
	return 0;
}
int		MySQLMsgConverter::Select(std::string & sql, std::vector<std::string> * fields , const char * where_ ){
	//
	sql = "SELECT ";
	if (fields){
		for (int i = 0; i < (int)fields->size(); ++i){
			if (i == 0){
				sql += ",";
			}
			sql += fields->at(i);
		}
	}
	else {
		for (int i = 0; i < (int)desc->field_count(); ++i){
			if (i == 0){
				sql += ",";
			}
			sql += desc->field(i)->name();
		}
	}
	sql += " FROM `";
	sql += GetTableName();
	sql += "` WHERE";
	if (where_){
		sql += where_;
	}
	else {
		int is_first = 0;
		int is_pk = 0;
		std::vector<std::pair<std::string, std::string > > kvlist;
		int ret = GetFieldsSQLKList(kvlist);
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
int		MySQLMsgConverter::Delete(std::string & sql, const char * where_ ){
	//
	sql = "DELETE ";
	sql += " FROM `";
	sql += GetTableName();
	sql += "` WHERE";
	if (where_){
		sql += where_;
	}
	else {
		int is_first = 0;
		int is_pk = 0;
		std::vector<std::pair<std::string, std::string > > kvlist;
		int ret = GetFieldsSQLKList(kvlist);
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
int		MySQLMsgConverter::Replace(std::string & sql){
	//REPLACE INTO 
	sql = "REPLACE INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = GetFieldsSQLKList(kvlist);
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
int		MySQLMsgConverter::Update(std::string & sql){
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
	int ret = GetFieldsSQLKList(kvlist);
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
int		MySQLMsgConverter::Insert(string & sql){
	//INSERT INTO order_(...)	values
	//update order_()	set		where
	sql = "INSERT INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = GetFieldsSQLKList(kvlist);
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


