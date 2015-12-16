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

int	MySQLMsgMeta::AttachMsg(const google::protobuf::Message *	msg_, bool flatmode){
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
	if (meta.m_divnum > 0){
		uint64_t ullspkey = atoll(cvt->GetMsgFieldValue(*msg, meta.m_divkey.c_str()).c_str());
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
int				MySQLMsgMeta::Select(std::string & sql, std::vector<std::string> * fields, const char * where_, bool flatmode){
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
		sql += "*";
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

		int ret = 0;
		if (flatmode){
			ret = cvt->GetMsgSQLFlatKVList(*msg, kvlist, "");
		}
		else {
			ret = cvt->GetMsgSQLKVList(*msg, kvlist);
		}
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
int		MySQLMsgMeta::Delete(std::string & sql, const char * where_, bool flatmode){
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

		int ret = 0;
		if (flatmode){
			ret = cvt->GetMsgSQLFlatKVList(*msg, kvlist, "");
		}
		else {
			ret = cvt->GetMsgSQLKVList(*msg, kvlist);
		}
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
int		MySQLMsgMeta::Replace(std::string & sql, bool flatmode){
	//REPLACE INTO 
	sql = "REPLACE INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = 0;
	if (flatmode){
		ret = cvt->GetMsgSQLFlatKVList(*msg, kvlist, "");
	}
	else {
		ret = cvt->GetMsgSQLKVList(*msg, kvlist);
	}
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
int		MySQLMsgMeta::Update(std::string & sql, bool flatmode){
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
	int ret = 0;
	if (flatmode){
		ret = cvt->GetMsgSQLFlatKVList(*msg, kvlist, "");
	}
	else {
		ret = cvt->GetMsgSQLKVList(*msg, kvlist);
	}
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
int		MySQLMsgMeta::Insert(std::string & sql, bool flatmode){
	//INSERT INTO order_(...)	values
	//update order_()	set		where
	sql = "INSERT INTO `";
	sql += GetTableName();
	sql += "` (";
	std::vector<std::pair<string, string> > kvlist;
	int ret = 0;
	if (flatmode){
		ret = cvt->GetMsgSQLFlatKVList(*msg, kvlist, "");
	}
	else {
		ret = cvt->GetMsgSQLKVList(*msg, kvlist);
	}
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
int		MySQLMsgMeta::DropTable(std::string & sql){
	sql = "DROP TABLE IF EXISTS `";
	sql += GetTableName();
	sql += "`;";
	return 0;
}

int		MySQLMsgMeta::CreateTable(std::string & sql, bool flatmode){
	if (flatmode){
		return cvt->CreateFlatTables(msg_desc->name().c_str(), sql, GetTableIdx());
	}
	return cvt->CreateTables(msg_desc->name().c_str(), sql, GetTableIdx());
}


///////////////////////////////////////////////////////////////////////////////////////////

MySQLMsgCvt::MySQLMsgCvt(const string & file, st_mysql * pMysql, size_t MAX_FIELD_BUFFER) :meta_file(file), mysql(pMysql){
	field_buffer.reserve(MAX_FIELD_BUFFER);//1MB
	escaped_buffer.reserve(MAX_FIELD_BUFFER * 2 + 1);
}
int		MySQLMsgCvt::SetMsgFieldMySQLValue(google::protobuf::Message & msg, const std::string & key, const char * value, size_t value_length){
	const Reflection * pReflection = msg.GetReflection();
	auto msg_desc = msg.GetDescriptor();
	const FieldDescriptor * pField = msg_desc->FindFieldByName(key);
	if (!pField){
		cerr << "not found the msg field from mysql field name :" << key << endl;
		return -1; //not found
	}
	if (pField->is_repeated()){
		//not support
		cerr << "not support repeat field in top layer unfolding mode ! field:" << pField->name() << endl;
		return -1;
	}
	/////////////////////////not repeat . scalar ////////////////////////////////////////
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
	cerr << "not found field in meta desc key:" << key << " msg type:" << msg_desc->name() << endl;
	return 0;
}
string	MySQLMsgCvt::GetMsgFieldValue(const google::protobuf::Message & msg, const char * key){
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
int		MySQLMsgCvt::GetMsgSQLKV(const google::protobuf::Message & msg, const FieldDescriptor * pField, std::pair<std::string, std::string> & kv){
	kv.first = pField->name();
	kv.second = "";
	field_buffer[0] = 0;
	size_t buffer_len = 0;
	const Reflection * pReflection = msg.GetReflection();
	bool need_escape = false;
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
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "'%s'", pReflection->GetString(msg, pField).c_str());
		need_escape = true;
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
			need_escape = true;
		}
		else {
			*((char*)field_buffer.data()) = '\'';
			*((char*)field_buffer.data() + 1) = '\'';
			buffer_len = 2;
		}
	default:
		cerr << "unkown type ! field:" << pField->name() << " type: " << pField->cpp_type() << endl;
		return -2;
	}
	assert(buffer_len != 0);
	//need escape
	if (mysql && need_escape && buffer_len > 2){
		bzero((char*)escaped_buffer.data() + buffer_len,
			buffer_len + 1); //
		mysql_real_escape_string(mysql, (char*)&escaped_buffer[1], (char*)field_buffer.data() + 1, buffer_len - 2);
		kv.second.assign(escaped_buffer.data());
		kv.second.append("'");
	}
	else {
		kv.second.assign(field_buffer.data(), buffer_len);
	}
	return 0;
}

int		MySQLMsgCvt::GetMsgSQLKVList(const google::protobuf::Message & msg, std::vector<std::pair<string, string> > & values, bool readmode )
{
	values.clear();
	auto msg_desc = msg.GetDescriptor();
	for (int i = 0; i < msg_desc->field_count(); ++i)
	{
		const FieldDescriptor * pField = msg_desc->field(i);
		if (!pField->is_repeated() &&
			!msg.GetReflection()->HasField(msg, pField)){
			continue;
		}
		std::pair<std::string, std::string> kv;
		if (pField->is_repeated()){
			//unfold ? no need support
			cerr << "no support a repeated field !" << pField->name() << " type:" << msg.GetTypeName() << endl;
			return -1;
		}
		else {
			if (GetMsgSQLKV(msg, pField, kv)){
				cerr << "get field kv error ! field name:" << pField->name() << " type:" << msg.GetTypeName() << endl;
				return -2;
			}
		}
		values.push_back(kv);
	}
	return 0;
}
int		MySQLMsgCvt::CheckMsgValid(const google::protobuf::Descriptor * msg_desc, bool root, bool flatmode ){
	EXTMessageMeta meta;
	if (meta.AttachDesc(msg_desc)){
		//error parse from desc
		cerr << "error parse from desc " << endl;
		return -1;
	}
	if (root){
		if (meta.pks_name.empty()){
			//not found the pk def
			cerr << "not found the pk def " << endl;
			return -2;
		}
		if (meta.pks_name.size() != meta.pks_fields.size()){
			cerr << "meta pks size not match " << endl;
			return -3;
		}
	}
	//check field type
	for (auto & sfm : meta.sub_fields){
		if (flatmode){
			if (sfm.field_desc->is_repeated() && sfm.z_count <= 0){
				cerr << "not db repeat field but no count define! error field :" << sfm.field_desc->name() << " count:" << sfm.f_count << endl;
				return -4;
			}
			if (sfm.field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
				int ret = CheckMsgValid(sfm.field_desc->message_type(), false, true);
				if (ret){
					return -5;
				}
			}
		}
		else {
			if (sfm.field_desc->is_repeated()){
				cerr << "found a repeat field in not a flatmode :" << sfm.field_desc->name() << endl;
				return -6;
			}
		}
	}
	if (root){
		if (meta.m_divnum < 0){
			//must be a integer 
			error_stream << "meta divnum is error :" << meta.m_divnum << endl;
			return -7;
		}
		if (!meta.m_divkey.empty() && !msg_desc->FindFieldByName(meta.m_divkey)){
			cerr << "msg :" << msg_desc->name() << " div key:" << meta.m_divkey << " not found !" << endl;
			return -8;
		}
	}
	return 0;
}
int		MySQLMsgCvt::InitMeta(int n , const char ** path ){
    if (protometa.Init(path, n)){
		error_stream << "proto meta init error !" << endl;
		return -1;
	}
    auto filedesc = protometa.LoadFile(meta_file.c_str());
    if (!filedesc){
		error_stream << "proto meta load error !" << endl;
		return -2;
	}
    package_name = filedesc->package();
    return 0;
}
#define TABLE_NAME_POSTFIX		("_")
#define TABLE_REPEATED_FIELD_POSTFIX	("$")
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
string			MySQLMsgCvt::GetRepeatedFieldLengthName(const std::string & name){
	string str_field_name = name;
	str_field_name += TABLE_REPEATED_FIELD_POSTFIX;
	str_field_name += "count";
	return str_field_name;
}
string			MySQLMsgCvt::GetRepeatedFieldName(const std::string & name, int idx){
	string str_field_name = name;
	str_field_name += TABLE_REPEATED_FIELD_POSTFIX;
	str_field_name += to_string(idx);
	return str_field_name;
}
bool		MySQLMsgCvt::IsRepeatedFieldLength(const std::string & field_name, const std::string & key){
	return (key == GetRepeatedFieldLengthName(field_name));
}
int			MySQLMsgCvt::GetRepeatedFieldIdx(const std::string & field_name, const std::string & key){
	string::size_type fpos = key.find(field_name);
	if (fpos == string::npos){
		return -1;
	}
	fpos += field_name.length();
	if (key.substr(fpos, strlen(TABLE_REPEATED_FIELD_POSTFIX)) != TABLE_REPEATED_FIELD_POSTFIX){
		return -2;
	}
	fpos += strlen(TABLE_REPEATED_FIELD_POSTFIX);
	string sidx = key.substr(fpos);
	if (sidx.empty() || sidx[0] < '0' || sidx[0] > '9'){
		return -3;
	}
	return stoi(sidx);
}


int			MySQLMsgCvt::GetMsgFlatTableSQLFields(const google::protobuf::Descriptor * msg_desc, std::string & sql, const std::string & prefix){
	EXTMessageMeta	meta;
	if (meta.AttachDesc(msg_desc)){
		return -1;
	}
	for (auto & field : meta.sub_fields){
		//sql += "`" + GetRepeatedFieldLengthName(field.field_desc->name()) + "` ";
		//sql += "INT UNSIGNED NOT NULL";
		if (field.field_desc->is_repeated()){
			//is repeated , msg or not , count is first
			sql += "`" + prefix + GetRepeatedFieldLengthName(field.field_desc->name()) + "` ";
			sql += "INT UNSIGNED NOT NULL";
			sql += ",\n";		
			for (int i = 0; i < field.z_count; ++i){
				//prefix
				string field_prefix = prefix + GetRepeatedFieldName(field.field_desc->name(), i);
				if (field.field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
					if (GetMsgFlatTableSQLFields(field.field_desc->message_type(), sql,
						field_prefix + TABLE_REPEATED_FIELD_POSTFIX)){
						return -2;
					}
				}
				else {
					sql += "`" + field_prefix + "` " + field.GetMysqlFieldType();
					sql += " NOT NULL";
					sql += ",\n";
				}
			}
		}
		else if (field.field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
			//unflod
			if (GetMsgFlatTableSQLFields(field.field_desc->message_type(), sql,
				prefix + field.field_desc->name() + TABLE_REPEATED_FIELD_POSTFIX)){
				return -3;
			}
		}
		else {
			sql += "`" + prefix + field.field_desc->name() + "` " + field.GetMysqlFieldType();
			sql += " NOT NULL";
			sql += ",\n";
		}
	}
	return 0;
}

int			MySQLMsgCvt::CreateFlatTables(const char * msg_type, std::string & sql, int idx){
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
	//append table sql fields type defines	
	GetMsgFlatTableSQLFields(msg_desc, sql, "");

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

int			MySQLMsgCvt::CreateTables(const char * msg_type, std::string & sql,int idx ){
    string msg_type_name = msg_type;
    if (!package_name.empty()){
        msg_type_name = package_name + ".";
        msg_type_name += msg_type;
    }
    auto msg_desc = protometa.GetMsgDesc(msg_type_name.c_str());
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
int			MySQLMsgCvt::GetMsgFromSQLRow(google::protobuf::Message & msg, const MySQLRow &  sql_row, bool flatmode ){
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
	if (flatmode){		
		ret = SetMsgSQLFlatKVList(msg, sql_row);
	}
	else {
		for (int i = 0; i < sql_row.num_fields; ++i){
			ret = SetMsgFieldMySQLValue(msg,
				std::string(sql_row.fields_name[i]),
				sql_row.row_data[i],
				sql_row.row_lengths[i]);
			if (ret){
				cerr << "set field :" << sql_row.fields_name[i] << " value error !" << endl;
				break;
			}
		}
	}
	if (ret){
		return -3;
	}
	return 0;
}
int			MySQLMsgCvt::GetMsgBufferFromSQLRow(char * buffer, int * buffer_len, const MySQLRow &  sql_row, bool flatmode){
	std::string msg_type_name = GetMsgTypeNameFromTableName(sql_row.table_name);
    Message * pMsg = protometa.NewDynMessage(msg_type_name.c_str());
	if (!pMsg){
		cerr << "not found message for table name:" << msg_type_name << endl;
		return -1;
	}
	int ret = GetMsgFromSQLRow(*pMsg, sql_row, flatmode);
	if (ret){
		ret = -1 + ret;
		goto FAIL_CONV;
	}
	if (*buffer_len < pMsg->ByteSize()){
		cerr << "the buffer length: " << *buffer_len <<  " is too few for object pack byte size:" << pMsg->ByteSize() << endl;
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
int		
MySQLMsgCvt::GetMsgSQLFlatKVRepeated(const google::protobuf::Message & msg, 
	const google::protobuf::Reflection * pReflection,
	const google::protobuf::FieldDescriptor * pField,
	int idx, std::vector<std::pair<std::string, std::string> > & values, const string & prefix){
	//get idx value
	std::pair<std::string, std::string> kv;
	kv.first = prefix + pField->name();
	field_buffer[0] = 0;
	size_t buffer_len = 0;
	bool need_escape = false;
	switch (pField->cpp_type())
	{
	case FieldDescriptor::CPPTYPE_FLOAT:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%f", pReflection->GetRepeatedFloat(msg, pField, idx));
		break;
	case FieldDescriptor::CPPTYPE_DOUBLE:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%lf", pReflection->GetRepeatedDouble(msg, pField, idx));
		break;
	case FieldDescriptor::CPPTYPE_INT32:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetRepeatedInt32(msg, pField, idx));
		break;
	case FieldDescriptor::CPPTYPE_INT64:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%ld", pReflection->GetRepeatedInt64(msg, pField, idx));
		break;
	case FieldDescriptor::CPPTYPE_UINT32:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%u", pReflection->GetRepeatedUInt32(msg, pField, idx));
		break;
	case FieldDescriptor::CPPTYPE_UINT64:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%lu", pReflection->GetRepeatedUInt64(msg, pField, idx));
		break;
	case FieldDescriptor::CPPTYPE_ENUM:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetRepeatedEnum(msg, pField, idx)->number());
		break;
	case FieldDescriptor::CPPTYPE_BOOL:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetRepeatedBool(msg, pField, idx) ? 1 : 0);
		break;
	case FieldDescriptor::CPPTYPE_STRING:
		buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "'%s'", pReflection->GetString(msg, pField).c_str());
		need_escape = true;
		break;
	default:
		cerr << "unkown type ! field:" << pField->name() << " type: " << pField->cpp_type() << endl;
		return -100;
	}
	assert(buffer_len > 0);
	//need escape
	if (mysql && need_escape && buffer_len > 2){
		memset((char*)escaped_buffer.data() + buffer_len, 0,
			buffer_len + 1); //
		mysql_real_escape_string(mysql, (char*)&escaped_buffer[1], (char*)field_buffer.data() + 1, buffer_len - 2);
		kv.second.assign(escaped_buffer.data());
		kv.second.append("'");
	}
	else {
		kv.second.assign(field_buffer.data(), buffer_len);
	}
	values.push_back(kv);
	return 0;
}
int		MySQLMsgCvt::GetMsgSQLFlatKVList(const google::protobuf::Message & msg, std::vector<std::pair<std::string, std::string> > & values, const string & prefix){
	auto msg_desc = msg.GetDescriptor();
	const Reflection * pReflection = msg.GetReflection();
	for (int i = 0; i < msg_desc->field_count(); ++i){
		const FieldDescriptor * pField = msg_desc->field(i);
		if (!pField->is_repeated() &&
			!pReflection->HasField(msg, pField)){
			continue;
		}
		std::pair<std::string, std::string> kv;
		if (pField->is_repeated()){
			const Reflection * pReflection = msg.GetReflection();
			int rep_count = pReflection->FieldSize(msg, pField);//max count be extension options
			if (rep_count == 0){
				continue;
			}
			kv.first = prefix + GetRepeatedFieldLengthName(pField->name());
			kv.second = to_string(rep_count);//must be uint32
			values.push_back(kv);
			for (int i = 0; i < rep_count; ++i){
				if (GetMsgSQLFlatKVRepeated(msg, msg.GetReflection(), pField, i, values, prefix + GetRepeatedFieldName(pField->name(), i))){
					cerr << "get field kv error ! field name:" << pField->name() << " type:" << msg.GetTypeName() << endl;
					return -1;
				}
			}
		}
		else if (pField->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
			//unfold
			const Message & submsg = pReflection->GetMessage(msg, pField);
			if (GetMsgSQLFlatKVList(submsg, values, prefix + pField->name() + TABLE_REPEATED_FIELD_POSTFIX)){
				cerr << "get field kv error ! field name:" << pField->name() << " type:" << msg.GetTypeName() << endl;
				return -2;
			}
		}
		else {
			//basic type
			std::pair<std::string, std::string> kv;
			kv.first = prefix + pField->name();
			field_buffer[0] = 0;
			size_t buffer_len = 0;
			bool need_escape = false;
			switch (pField->cpp_type())
			{
			case FieldDescriptor::CPPTYPE_FLOAT:
				buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%f", pReflection->GetFloat(msg, pField));
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
				buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "%d", pReflection->GetBool(msg, pField) ? 1 : 0);
				break;
			case FieldDescriptor::CPPTYPE_STRING:
				buffer_len = snprintf((char*)field_buffer.data(), field_buffer.capacity(), "'%s'", pReflection->GetString(msg, pField).c_str());
				need_escape = true;
				break;
			default:
				cerr << "unkown type ! field:" << pField->name() << " type: " << pField->cpp_type() << endl;
				return -100;
			}
			assert(buffer_len > 0);
			//need escape
			if (mysql && need_escape && buffer_len > 2){
				memset((char*)escaped_buffer.data() + buffer_len, 0,
					buffer_len + 1); //
				mysql_real_escape_string(mysql, (char*)&escaped_buffer[1], (char*)field_buffer.data() + 1, buffer_len - 2);
				kv.second.assign(escaped_buffer.data());
				kv.second.append("'");
			}
			else {
				kv.second.assign(field_buffer.data(), buffer_len);
			}
			values.push_back(kv);
		}
	}
	return 0;
}
int		MySQLMsgCvt::SetMsgSQLFlatKVList(google::protobuf::Message & msg, const MySQLRow &  sql_row){
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
		ret = SetMsgSQLFlatKV(msg,
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
int		MySQLMsgCvt::SetMsgSQLFlatKVRepeated(google::protobuf::Message & msg, 
	const google::protobuf::Reflection * pReflection, 
	const google::protobuf::FieldDescriptor * pField, int idx, const std::string & key, const char * value, size_t value_length){
	for (int i = pReflection->FieldSize(msg, pField); i <= idx; ++i){
		switch (pField->cpp_type())
		{
		case FieldDescriptor::CPPTYPE_FLOAT:
			pReflection->AddFloat(&msg, pField, 0);
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			pReflection->AddDouble(&msg, pField, 0);
			break;
		case FieldDescriptor::CPPTYPE_INT32:
			pReflection->AddInt32(&msg, pField, 0);
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			pReflection->AddInt64(&msg, pField, 0LL);
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			pReflection->AddUInt32(&msg, pField, 0U);
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			pReflection->AddUInt64(&msg, pField, 0ULL);
			break;
		case FieldDescriptor::CPPTYPE_ENUM:
			do {
				auto evdesc = pField->enum_type()->FindValueByNumber(0);
				if (evdesc){
					pReflection->AddEnum(&msg, pField, evdesc);
				}
				else {
					cerr << "not found the enum value:" << value << "! field name:" << pField->name() << " msg type:" << msg.GetTypeName() << endl;
					return -1;
				}
			} while (false);
			break;
		case FieldDescriptor::CPPTYPE_BOOL:
			pReflection->AddBool(&msg, pField, false);
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			pReflection->AddString(&msg, pField, "");
			break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
			pReflection->AddMessage(&msg, pField);
			break;
		default:
			return -100;
		}
	}
	switch (pField->cpp_type())
	{
	case FieldDescriptor::CPPTYPE_FLOAT:
		pReflection->SetRepeatedFloat(&msg, pField, idx, atof(value));
		return 0;
	case FieldDescriptor::CPPTYPE_DOUBLE:
		pReflection->SetRepeatedDouble(&msg, pField, idx, atof(value));
		return 0;
	case FieldDescriptor::CPPTYPE_INT32:
		pReflection->SetRepeatedInt32(&msg, pField, idx, atoi(value));
		return 0;
	case FieldDescriptor::CPPTYPE_INT64:
		pReflection->SetRepeatedInt64(&msg, pField, idx, atoll(value));
		return 0;
	case FieldDescriptor::CPPTYPE_UINT32:
		pReflection->SetRepeatedUInt32(&msg, pField, idx, atoi(value));
		return 0;
	case FieldDescriptor::CPPTYPE_UINT64:
		pReflection->SetRepeatedUInt64(&msg, pField, idx, atoll(value));
		return 0;
	case FieldDescriptor::CPPTYPE_ENUM:
		do {
			auto evdesc = pField->enum_type()->FindValueByNumber(atoi(value));
			if (evdesc){
				pReflection->SetRepeatedEnum(&msg, pField, idx, evdesc);
			}
			else {
				cerr << "not found the enum value:" << value << "! field name:" << pField->name() << " msg type:" << msg.GetTypeName() << endl;
				return -1;
			}
		} while (false);
		return 0;
	case FieldDescriptor::CPPTYPE_BOOL:
		pReflection->SetRepeatedBool(&msg, pField, idx, atoi(value) != 0 ? true : false);
		return 0;
	case FieldDescriptor::CPPTYPE_STRING:
		pReflection->SetRepeatedString(&msg, pField, idx, std::string(value, value_length));
		return 0;
	case FieldDescriptor::CPPTYPE_MESSAGE:
		do{
			auto pSubMsg = pReflection->MutableRepeatedMessage(&msg, pField, idx);
			if (pSubMsg){
				return SetMsgSQLFlatKV(*pSubMsg, key, value, value_length);
			}
			else {
				cerr << "mutable sub message error ! field name:" << pField->name() << " msg type:" << msg.GetTypeName() << endl;
				return -3;
			}

		} while (false);
		return 0;
	default:
		return -100;
	}
}
int		MySQLMsgCvt::SetMsgSQLFlatKV(google::protobuf::Message & msg, const std::string & key, 
	const char * value, size_t value_length){
	const Reflection * pReflection = msg.GetReflection();
	auto msg_desc = msg.GetDescriptor();
	const FieldDescriptor * pField = msg_desc->FindFieldByName(key);
	if (!pField){
		string::size_type field_pos = key.find(TABLE_REPEATED_FIELD_POSTFIX);
		string field_name = key.substr(0, field_pos);
		field_pos += strlen(TABLE_REPEATED_FIELD_POSTFIX); //field_idx_<>
		if (field_name.empty()){
			cerr << "not found the mysql field :" << key << " in msg:" << msg.GetTypeName() << endl;
			return -1;
		}
		pField = msg_desc->FindFieldByName(field_name);
		if (!pField){
			cerr << "not found the mysql field :" << key << " token 1st(field name):" << field_name << " in msg:" << msg.GetTypeName() << endl;
			return -2;
		}
		//-----------------------------------------------------------------------------------------------------------------------
		if (pField->is_repeated()){
			if (IsRepeatedFieldLength(pField->name(), key)){
				//extend size
				return 0; //no  need extend , lazy
			}
			string rep_idx_str = key.substr(field_pos, key.find(TABLE_REPEATED_FIELD_POSTFIX, field_pos));
			field_pos += strlen(TABLE_REPEATED_FIELD_POSTFIX);
			//must be indx
			int rep_idx = stoi(rep_idx_str);
			if (rep_idx >= 0){
				//$idx$v
				field_pos = key.find(TABLE_REPEATED_FIELD_POSTFIX, field_pos);
				field_pos += strlen(TABLE_REPEATED_FIELD_POSTFIX);
				//msg_set(k,v,idx)
				return SetMsgSQLFlatKVRepeated(msg, pReflection, pField, rep_idx, key.substr(field_pos), value, value_length);
			}
			else {
				cerr << "msg (" << msg.GetTypeName() << ")field is repeated , but mysql field name " << key <<  "not match !" << endl;
				return -3;
			}
		}
		else if (pField->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
			auto pSubMsg = pReflection->MutableMessage(&msg, pField);
			if (pSubMsg){
				//msg_set(k,v)
				return SetMsgSQLFlatKV(*pSubMsg, key.substr(field_pos), value, value_length);
			}
			else {
				cerr << "mutable sub message error ! field name:" << pField->name() << " msg type:" << msg_desc->name() << endl;
				return -3;
			}
		}
	}
	/////////////////////////not repeat . scalar ////////////////////////////////////////
	//single
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
	default:
		return -100;
	}
	cerr << "not found field in meta desc key:" << key << " msg type:" << msg_desc->name() << endl;
	return 0;
}
