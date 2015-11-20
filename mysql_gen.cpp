#include "mysql_gen.h"
#include "mysql/mysql.h"
#include "google/protobuf/compiler/importer.h"
#include <algorithm>
#include <functional>

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;




MySQLMsgMeta::MySQLMsgMeta(MySQLMsgCvt * pCvt):
			cvt(pCvt),msg_desc(nullptr),msg(nullptr){
}
void	MySQLMsgMeta::construct(){
	msg_desc = nullptr;
	msg = nullptr;
	table_idx = -1;
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
	
	if (!meta.m_divkey.empty() && !msg_desc->FindFieldByName(meta.m_divkey)){
		cerr << "msg :" << msg_desc->name() << " div key:" << meta.m_divkey << " not found !" << endl;
		return -8;
	}

	if (meta.m_divnum > 0){
		uint64_t ullspkey = atoll(cvt->GetFieldValue(*msg, meta.m_divkey.c_str()).c_str());
		table_idx = ullspkey % meta.m_divnum;
	}

	return 0;
}
int32_t			MySQLMsgMeta::GetTableIdx(){
	return table_idx;
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
			sql += "`";
			sql += fields->at(i);
			sql += "`";
		}
	}
	else {
		for (int i = 0; i < (int)msg_desc->field_count(); ++i){
			if (i != 0){
				sql += ",";
			}
			sql += "`";
			sql += msg_desc->field(i)->name();
			sql += "`";
		}
	}
	sql += " FROM `";
	sql += GetTableName();
	sql += "` ";
	if (where_){
		if (where_[0]){
			sql += " WHERE ";
			sql += where_;
		}
	}
	else {
		sql += " WHERE ";
		int is_first = 0;
		int is_pk = 0;
		std::vector<std::pair<std::string, std::string > > kvlist;
		int ret = cvt->GetMsgSQLKList(*msg, kvlist);
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
			sql += "`";
			sql += kv.first;
			sql += "`";
			sql += " = ";
			sql += kv.second;
		}
	}
	sql += ";";
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
		int ret = cvt->GetMsgSQLKList(*msg, kvlist);
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
			sql += " = ";
			sql += kv.second;
		}
	}
	sql += ";";
	return 0;
}
int		MySQLMsgMeta::Replace(std::string & sql){
	//REPLACE INTO 
	sql = "REPLACE INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = cvt->GetMsgSQLKList(*msg, kvlist);
	if (ret) return ret;
	for (int i = 0; i < (int)kvlist.size(); ++i)
	{
		if (i != 0)
		{
			sql += ",";
		}
		sql += "`";
		sql += kvlist[i].first;
		sql += "`";
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
	sql += ");";
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
	int ret = cvt->GetMsgSQLKList(*msg, kvlist);
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

		sql += '`' + kv.first + '`';
		sql += " = ";
		sql += kv.second;
	}
	if (!meta.m_autoinc.empty())
	{
		if (is_first != 0)
		{
			sql += " , ";
		}
		sql += '`' + meta.m_autoinc + '`';
		sql += " = ";
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

		sql += '`' + kv.first + '`';
		sql += " = ";
		sql += kv.second;
	}
	sql += ";";
	return 0;
}
int				MySQLMsgMeta::Insert(std::string & sql){
	//INSERT INTO order_(...)	values
	//update order_()	set		where
	sql = "INSERT INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = cvt->GetMsgSQLKList(*msg, kvlist);
	if (ret) return ret;
	for (int i = 0; i < (int)kvlist.size(); ++i)
	{
		if (i != 0)
		{
			sql += ",";
		}
		sql += "`";
		sql += kvlist[i].first;
		sql += "`";
	}
	if (!meta.m_autoinc.empty())
	{
		if (!kvlist.empty()){
			sql += ",";
		}
		sql += "`";
		sql += meta.m_autoinc;
		sql += "`";
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
	sql += ");";
	return 0;
}
int			MySQLMsgMeta::DropTable(std::string & sql){
	sql = "DROP TABLE IF EXISTS `";
	sql += GetTableName();
	sql += "`;";
	return 0;
}

int			MySQLMsgMeta::CreateTable(std::string & sql){
	return cvt->CreateTables(msg_desc->name().c_str(), sql, GetTableIdx());
}

///////////////////////////////////////////////////////////////////////////////////////////

MySQLMsgCvt::MySQLMsgCvt(const string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER) :meta_file(file), mysql(pMysql){
	field_buffer.reserve(MAX_FIELD_BUFFER);//1MB
	escaped_buffer.reserve(MAX_FIELD_BUFFER * 2 + 1);
}

int		MySQLMsgCvt::SetFieldValue(google::protobuf::Message & msg, const std::string & key, const char * value, size_t value_length){
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
			pReflection->SetFloat(&msg, pField, atof(value));
			return 0;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			pReflection->SetDouble(&msg, pField, atof(value));
			return 0;
		case FieldDescriptor::CPPTYPE_INT32:
			pReflection->SetInt32(&msg, pField, atoi(value));
			return 0;
		case FieldDescriptor::CPPTYPE_INT64:
			pReflection->SetInt64(&msg, pField, atoll(value));
			return 0;
		case FieldDescriptor::CPPTYPE_UINT32:
			pReflection->SetUInt32(&msg, pField, atoi(value));
			return 0;
		case FieldDescriptor::CPPTYPE_UINT64:
			pReflection->SetUInt64(&msg, pField, atoll(value));
			return 0;
		case FieldDescriptor::CPPTYPE_ENUM:
			do {
				auto evdesc = pField->enum_type()->FindValueByNumber(atoi(value));
				if (evdesc){
					pReflection->SetEnum(&msg, pField, evdesc);
				}
				else {
					cerr << "not found the enum value:" << value << "! field name:" << pField->name() << " msg type:" << msg_desc->name() << endl;
					return -1;
				}
			} while (false);
			return 0;
		case FieldDescriptor::CPPTYPE_BOOL:
			pReflection->SetBool(&msg, pField, atoi(value) != 0 ? true : false);
			return 0;
		case FieldDescriptor::CPPTYPE_STRING:
			pReflection->SetString(&msg, pField, std::string(value, value_length));
			return 0;
		case FieldDescriptor::CPPTYPE_MESSAGE:
			do{
				auto pSubMsg = pReflection->MutableMessage(&msg, pField);
				if (pSubMsg){
					if (!pSubMsg->ParseFromArray(value, value_length)){
						cerr << "parse mysql field error ! field name:" << pField->name() << " msg type:" << msg_desc->name() << endl;
						return -2;
					}
				}
				else {
					cerr << "mutable sub message error ! field name:" << pField->name() << " msg type:"<< msg_desc->name() << endl;
					return -3;
				}

			} while (false);
			return 0;
		default:
			return -100;
		}
	}
	cerr << "not found field in meta desc key:" << key << " msg type:" << msg_desc->name() << endl;
	return 0;
}
string	MySQLMsgCvt::GetFieldValue(const google::protobuf::Message & msg, const char * key){
	const Reflection * pReflection = msg.GetReflection();
	auto msg_desc = msg.GetDescriptor();
	string lower_case_name = key;
	std::transform(lower_case_name.begin(), lower_case_name.end(), lower_case_name.begin(), ::tolower);
	const FieldDescriptor * pField = msg_desc->FindFieldByLowercaseName(lower_case_name);
	if (!pField || pField->is_repeated()){
		cerr << "not found field (or is it a repeat field ? get value not support repeat field) :" << key << " lower case name:" << lower_case_name << endl;
		return "";
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
	default:
		cerr << "get value not support repeat field or message field:" << lower_case_name << " type:" << pField->type_name() << endl;
		return "";
	}
}
int		MySQLMsgCvt::GetFieldSQLKV(const google::protobuf::Message & msg, const FieldDescriptor * pField, std::pair<std::string, std::string> & kv){
	kv.first = pField->name();
	kv.second = "";
	field_buffer[0] = 0;
	size_t buffer_len = 0;
	const Reflection * pReflection = msg.GetReflection();
	switch (pField->cpp_type())
	{
	case FieldDescriptor::CPPTYPE_FLOAT:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(),"%f",pReflection->GetFloat(msg, pField));
		break;
	case FieldDescriptor::CPPTYPE_DOUBLE:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%lf", pReflection->GetDouble(msg, pField));
		break;
	case FieldDescriptor::CPPTYPE_INT32:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetInt32(msg, pField));
		break;
	case FieldDescriptor::CPPTYPE_INT64:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%ld", pReflection->GetInt64(msg, pField));
		break;
	case FieldDescriptor::CPPTYPE_UINT32:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%u", pReflection->GetUInt32(msg, pField));
		break;
	case FieldDescriptor::CPPTYPE_UINT64:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%lu", pReflection->GetUInt64(msg, pField));
		break;
	case FieldDescriptor::CPPTYPE_ENUM:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetEnum(msg, pField)->number());
		break;
	case FieldDescriptor::CPPTYPE_BOOL:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetBool(msg, pField)?1:0);
		break;
	case FieldDescriptor::CPPTYPE_STRING:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%s", pReflection->GetString(msg, pField).c_str());
		break;
	case FieldDescriptor::CPPTYPE_MESSAGE:
		if (pReflection->HasField(msg, pField)){
			const Message & _tmpmsg = pReflection->GetMessage(msg, pField);
			if (!_tmpmsg.SerializeToArray((char*)field_buffer.data() + 1, field_buffer.capacity() - 2)){
				cerr << "pack error ! field:" << pField->name() << " field buffer capacity:" << (int)(field_buffer.capacity() - 2) << " need :" << _tmpmsg.ByteSize() << endl;
				return -1;
			}
			*((char*)field_buffer.data()) = '\'';
			*((char*)field_buffer.data() + 1 + _tmpmsg.ByteSize()) = '\'';
			buffer_len = _tmpmsg.ByteSize() + 2;
		}
		break;
	default:
		cerr << "unkown type ! field:" << pField->name() << " type: " << pField->cpp_type() << endl;
		return -2;
	}
	if (buffer_len == 0){
		*((char*)field_buffer.data()) = '\'';
		*((char*)field_buffer.data() + 1) = '\'';
		buffer_len = 2;
	}
	//need escape
	if (mysql){
		char * buffer_msg = (char*)field_buffer.data();
		bzero((char*)escaped_buffer.data() + buffer_len,
			buffer_len + 1); //
		mysql_real_escape_string(mysql, (char*)&escaped_buffer[1], buffer_msg, buffer_len);
		kv.second.assign(escaped_buffer.data());
	}
	else {
		kv.second.assign(field_buffer.data(), buffer_len);
	}
	return 0;
}

int		MySQLMsgCvt::GetMsgSQLKList(const google::protobuf::Message & msg, std::vector<std::pair<string, string> > & values)
{
	values.clear();
	auto msg_desc = msg.GetDescriptor();
	for (int i = 0; i < msg_desc->field_count(); ++i)
	{
		const FieldDescriptor * pField = msg_desc->field(i);
		if (!pField){
			continue;
		}
		std::pair<std::string, std::string> kv;
		if (pField->is_repeated()){
			//unfold ? no need support
			cerr << "no support a repeated field !" << pField->name() << " type:" << msg.GetTypeName() << endl;
			return -1;
		}
		else {
			if (GetFieldSQLKV(msg, pField, kv)){
				cerr << "get field kv error ! field name:" << pField->name() << " type:" << msg.GetTypeName() << endl;
				return -2;
			}
		}
		values.push_back(kv);
	}
	return 0;
}
int		MySQLMsgCvt::InitSchema(){
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
#define TABLE_NAME_POSTFIX		("_")
std::string		MySQLMsgCvt::GetMsgTypeNameFromTableName(const std::string & table_name){
	string::size_type deli = table_name.find_last_of(TABLE_NAME_POSTFIX);
	if (deli == string::npos){
		return table_name;
	}
	else {
		return table_name.substr(0, deli);
	}
}
string		MySQLMsgCvt::GetTableName(const char * msg_type, int idx){
	string tbname = msg_type;
	if (idx >= 0){
		tbname += TABLE_NAME_POSTFIX;
		tbname += to_string(idx);
	}
	return tbname;
}
int			MySQLMsgCvt::CreateTables(const char * msg_type, std::string & sql,int idx ){
	auto msg_desc = protometa.GetMsgDesc(msg_type);
	if (!msg_desc){
		return -1;
	}
	EXTMessageMeta	meta;
	if (meta.AttachDesc(msg_desc)){
		return -2;
	}
	//template
	string table_name = GetTableName(msg_type, idx);
	sql = "CREATE TABLE IF NOT EXISTS `" + table_name + "` (";
	for (auto & field : meta.sub_fields)
	{
		sql += "`" + field.field_desc->name() + "` " + field.GetMysqlFieldType();
		sql += " NOT NULL";
		sql += ",\n";
	}
	string pks = "";
	for (auto & pk : meta.pks_name)
	{
		if (!pks.empty()){
			pks += ",";
		}
		pks += "`";
		pks += pk;
		pks += "`";
	}
	sql += "PRIMARY KEY (";
	sql += pks + ")\n";
	sql += ") ENGINE=InnoDB DEFAULT CHARSET utf8 COLLATE utf8_general_ci;";
	string sql_real = "";
	if (idx < 0 && meta.m_divnum > 0){
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
	sql += ";";
	return 0;
}

int			MySQLMsgCvt::CreateDB(const char * db_name, std::string & sql){
	sql = "CREATE DATABASE IF NOT EXISTS `";
	sql += db_name;
	sql += "` DEFAULT CHARSET utf8 COLLATE utf8_general_ci;";
	return 0;
}
int			MySQLMsgCvt::DropDB(const char * db_name, std::string & sql){
	sql = "DROP DATABASE IF EXISTS `";
	sql += db_name;
	sql += "`;";
	return 0;
}
int			MySQLMsgCvt::GetMsgFromMySQLRow(google::protobuf::Message & msg, const MySQLRow &  sql_row){
	if (sql_row.num_fields <= 0){
		//error number fields
		cerr << "errror number fields:" << sql_row.num_fields << endl;
		return -1;
	}
	string msg_type = msg.GetDescriptor()->name();
	std::string msg_type_name = GetMsgTypeNameFromTableName(sql_row.table_name);
	if (msg_type_name != msg_type){
		cerr << "type not matched ! expect type:" << msg_type << endl;
		return -2;
	}
	int ret = 0;
	for (int i = 0; i < sql_row.num_fields; ++i){
		ret = SetFieldValue(msg,
			std::string(sql_row.fields_name[i]),
			sql_row.row_data[i],
			sql_row.row_lengths[i]);
		if (ret){
			cerr << "set field value error !" << endl;
			return -3;
		}
	}
	return 0;
}
int			MySQLMsgCvt::GetMsgBufferFromMySQLRow(char * buffer, int * buffer_len, const MySQLRow &  sql_row){
	std::string msg_type_name = GetMsgTypeNameFromTableName(sql_row.table_name);
	Message * pMsg = protometa.NewDynMessage(msg_type_name.c_str());
	if (!pMsg){
		cerr << "not found message for table name:" << msg_type_name << endl;
		return -1;
	}
	int ret = GetMsgFromMySQLRow(*pMsg, sql_row);
	if (ret){
		ret = -1 + ret;
		goto FAIL_CONV;
	}
	if (*buffer_len < pMsg->ByteSize()){
		cerr << "the buffer length is too few for byte size:" << pMsg->ByteSize() << endl;
		ret = -1000;
		goto FAIL_CONV;
	}
	if (!pMsg->SerializeToArray(buffer, *buffer_len)){
		cerr << "pack msg error :" << pMsg->ByteSize() << endl;
		ret = -2000;
		goto FAIL_CONV;
	}
	*buffer_len = pMsg->ByteSize();
FAIL_CONV:
	protometa.FreeDynMessage(pMsg);
	return  ret;
}