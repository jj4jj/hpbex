
#include "google/protobuf/compiler/importer.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <queue>
#include <vector>
#include <sstream>
#include <string>
#include <exception>
#include <stdexcept>

using namespace std;

std::stringstream error_stream;
using namespace google::protobuf;
struct MFEC : compiler::MultiFileErrorCollector {
	void AddError(
				const string & filename,
				int line,
				int column,
				const string & message)
	{
		error_stream << "file name:" << filename << ":" << line << " error:" << message << endl;
	}
};

/*
optional string pks = 50000;
optional string divkey = 50001;
optional uint32 divnum = 50002;
optional string mcn = 50003;
optional string mdesc = 50004;
optional string relchk = 50005;
}
extend google.protobuf.FieldOptions {
optional string  cn = 50000;
optional string  desc = 50001;
optional string  count = 50002;
optional string  length = 50003;
}
extend google.protobuf.EnumValueOptions {
optional string ecn = 50000;
optional string edesc = 50001;
*/
static struct {
	const char * option;
	int tag;
}  option_map_tag[] = {
	{ "pks", 50000 },
	{ "divkey", 50001 },
	{ "divnum", 50002 },
	{ "mcn", 50003 },
	{ "mdesc", 50004 },
	{ "relchk", 50005 },

	///////////////////////////////////////
	{ "cn", 50000 },
	{ "desc", 50001 },
	{ "count", 50002 },
	{ "length", 50003 },

	///////////////////////////////////////
	{ "ecn", 50000 },
	{ "edesc", 50001 },
};
#define DIM_ARRAY(a)	(sizeof((a))/sizeof((a)[0]))


template<class D>
int GetDescOption(string & value, D desc, const string & option){
	int tag = 0;
	value = "";
	for (int i = 0; i < DIM_ARRAY(option_map_tag); ++i){
		if (strcmp(option.c_str(), option_map_tag[i].option) == 0){
			tag = option_map_tag[i].tag;
		}
	}
	if (!tag){
		error_stream << "not found the option->tag map option:" << option.c_str() << " in desc:" << desc->name() << endl;
		return -1;
	}
	auto options = desc->options();
	for (int j = 0; j < options.unknown_fields().field_count(); ++j){
		auto ouf = options.unknown_fields().field(j);
		if (ouf.number() == tag){ //cn
			value = ouf.length_delimited();
			//clog << "found the option:" << option.c_str() << " tag:" << tag << " value:"<< value << " in desc:" << desc->name() << endl;
			return 0;
		}
	}
	error_stream << "not found the option:" << option.c_str() << " tag:" << tag << " in desc:" << desc->name() << endl;
	return -2;
}
/////////////////////////////////////////////////////////////////////////////////////////////
struct STMetaUtil {
	static string GetStructName(const Descriptor * desc){
		string sname = desc->name();
		sname += "_ST";
		return sname;
	}
};



struct STFieldMeta {
	const FieldDescriptor * field_desc;
	string count;
	string length;
	string cn;
	string desc;

	int	    ParseFrom(const FieldDescriptor * fd){
		field_desc = fd;
		if (fd->is_repeated()){
			//cout > 0
			if (GetDescOption(count, fd, "count")){
				error_stream << "the field:" << fd->name() << " in message:" << fd->type_name() << " is a repeat , but not found options [count] !" << endl;
				return -1;
			}
		}
		if (fd->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
			if (GetDescOption(length, fd, "length")){
				error_stream << "the field:" << fd->name() << " in message:" << fd->type_name() << " is a string , but not found options [length] !" << endl;
				return -2;
			}
		}
		return 0;
	}
	string GetScalarTypeName(){
		static const char * s_var_type_names[] = { "", "int32_t", "uint32_t", "int64_t", "uint64_t", "double", "float", "const char *", "int32_t", "Message" };
		const char * pszTypeName = s_var_type_names[field_desc->cpp_type()];
		if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_ENUM){
			return field_desc->enum_type()->name();
		}
		else if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
			return STMetaUtil::GetStructName(field_desc->message_type());
		}
		return pszTypeName;
	}
	string GetTypeName() {
		const char * pszTypeName = GetScalarTypeName().c_str();
		if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
			if (field_desc->type() == FieldDescriptor::TYPE_STRING){
				string buffer_type_name = "struct { char data[";
				buffer_type_name += length;
				buffer_type_name += "]; }";
				return buffer_type_name;
			}
			else {
				string buffer_type_name = "struct { size_t length; uint8_t data[";
				buffer_type_name += length;
				buffer_type_name += "]; }";
				return buffer_type_name;
			}
		}
		else {
			return pszTypeName;
		}
	}
	string GetVarName() {
		//static const char * type_prefix = ["", "i", "ll", "dw", "ull", "df", "f", "b", "en", "str", "st"];
		//desc->camelcase_name();
		return field_desc->lowercase_name();
	}
	string GetScalarConvToMeth(const char * convtomsg_, const string & st_var_name, const string & msg_var_name){
		auto fmt = field_desc->message_type();
		string meth = "";
		string mutable_meth = "set_";
		if (fmt){
			mutable_meth = "mutable_";
		}
		if (field_desc->is_repeated()){
			mutable_meth = "add_";
		}
		if (fmt){
			meth += st_var_name;
			meth += ".convto(*";
			meth += convtomsg_;
			meth += "." + mutable_meth;
			meth += msg_var_name;
			meth += "()";
		}
		else {
			meth += convtomsg_;
			meth += "." + mutable_meth;
			meth += msg_var_name;
			meth += "(";
			meth += st_var_name;
			if (field_desc->type() == FieldDescriptor::TYPE_STRING){
				meth += ".data";
			}
			else if (field_desc->type() == FieldDescriptor::TYPE_BYTES){
				meth += ".data,";
				meth += st_var_name;
				meth += ".length";
			}
		}
		meth += ")";
		return meth;
	}
	string GetScalarConvFromMeth(const char * convtomsg_, const string & st_var_name, const string & msg_var_name){
		auto fmt = field_desc->message_type();
		string meth = "";
		if (fmt){
			meth += st_var_name;
			meth += ".convfrom(";
			meth += convtomsg_;
			meth += ".";
			meth += msg_var_name;
			meth += ")";
		}
		else {
			if (field_desc->type() == FieldDescriptor::TYPE_STRING){
				meth = "strncpy(";
				meth += st_var_name.c_str();
				meth += ".data, ";
				meth += convtomsg_;
				meth += ".";
				meth += msg_var_name;
				meth += ".data(), ";
				meth += length.c_str();
				meth += "-1)";
			}
			else if (field_desc->type() == FieldDescriptor::TYPE_BYTES){
				meth = "memcpy(";
				meth += st_var_name.c_str();
				meth += ".data, ";
				meth += convtomsg_;
				meth += ".";
				meth += msg_var_name;
				meth += ".data(), std::min(";
				meth += length.c_str();
				meth += ", ";
				meth += convtomsg_;
				meth += ".";
				meth += msg_var_name + ".length))";
			}
			else {
				meth = st_var_name;
				meth += " = ";
				meth += convtomsg_;
				meth += ".";
				meth += msg_var_name;
			}
		}
		return meth;
	}
};

struct STMessageMeta {
	const Descriptor * msg_desc;
	string pks;
	string divkey;
	string divnum;
	string mcn;
	string mdesc;
	string relchk;
	std::vector<STFieldMeta*> pks_fields;
	std::vector<STFieldMeta> sub_fields;
	int	    ParseFrom(const Descriptor * desc){
		msg_desc = desc;
#define GET_DESC_OPTION(pks)	GetDescOption(pks, msg_desc, #pks)

		GET_DESC_OPTION(pks);
		GET_DESC_OPTION(divkey);
		GET_DESC_OPTION(divnum);
		GET_DESC_OPTION(mcn);
		GET_DESC_OPTION(mdesc);
		GET_DESC_OPTION(relchk);

		////////////////////////////
		ParseSubFields();
		int ret = ParsePKS();

		return ret;
	}
	void	ParseSubFields(){
		for (int i = 0; i < msg_desc->field_count(); ++i){
			STFieldMeta sfm;
			sfm.ParseFrom(msg_desc->field(i));
			sub_fields.push_back(sfm);
		}
	}
	int		ParsePKS(){
		string::size_type bpos = 0, fpos = 0;
		while (true && !pks.empty()){
			fpos = pks.find(',', bpos);
			string f_field_name = "";
			if (fpos != string::npos && fpos > bpos){
				f_field_name = pks.substr(bpos, fpos - bpos);
			}
			else {
				if (fpos != bpos){
					f_field_name = pks.substr(bpos);
				}
			}		
			for (auto & suf : sub_fields){
				if (suf.field_desc->name() == f_field_name){
					pks_fields.push_back(&suf);
				}
			}
			if (fpos == string::npos){
				break;
			}
			bpos = fpos + 1;
		}
		//all fields
		if (pks_fields.empty()){
			for (auto & suf : sub_fields){
				pks_fields.push_back(&suf);
			}
		}
		return 0;
	}
};


class GenerateCXXFlat {
	typedef std::unordered_map<const Descriptor *, int>										MsgDegree;
	typedef	std::unordered_map<const Descriptor *, std::unordered_set<const Descriptor *> >	ReverseRef;
	typedef ReverseRef::iterator		ReverseRefItr;

	ReverseRef	reverse_refer;
	MsgDegree	msg_degrees;
	const Descriptor * root;
	std::vector<const Descriptor*>	ordered_desc;
public:
	GenerateCXXFlat(const Descriptor * desc){		
		root = desc;
	}
	static string repeat(const char * s , int n){
		string str = "";
		for (int i = 0; i < n; ++i){
			str += s;
		}
		return str;
	}
	string ConvertMsg(const Descriptor * desc, const char * indent = "    "){	
		stringstream ss;
		int level = 0;
		char sz_line_buffer[1024];
		#define	WRITE_LINE(format, ...)		do{\
			snprintf(sz_line_buffer, sizeof(sz_line_buffer), format, ##__VA_ARGS__); \
			ss << GenerateCXXFlat::repeat(indent, level) << sz_line_buffer << std::endl; \
				}while (false)
		#define	WRITE_STR(format, ...)		do{\
			snprintf(sz_line_buffer, sizeof(sz_line_buffer), format, ##__VA_ARGS__); \
			ss << GenerateCXXFlat::repeat(indent, level) << sz_line_buffer; \
				}while (false)		
		////////////////////////////////////////////////////////////////////////////////////
		//struct Msg {
		WRITE_LINE("struct %s {", STMetaUtil::GetStructName(desc).c_str());
		level++;
		std::vector<STFieldMeta>	fieldmeta_list;

		STMessageMeta	msg_meta;
		if (msg_meta.ParseFrom(desc)){
			throw logic_error("parse error !");
		}

		for (auto & sfm : msg_meta.sub_fields){
			if (sfm.count.empty()){
				WRITE_LINE("%s\t\t%s;", sfm.GetTypeName().c_str(), sfm.GetVarName().c_str());
			}
			else {
				WRITE_LINE("struct {size_t count; %s list[%s];}\t\t%s;",
					sfm.GetTypeName().c_str(), sfm.count.c_str(), sfm.GetVarName().c_str());
			}
			fieldmeta_list.push_back(sfm);
		}
		///////////////////////////////////////////////////////////////////////////
		WRITE_LINE("\t");
		WRITE_LINE("//////////////member functions///////////////////////////////////");
		//construct
		WRITE_LINE("void\t\tconstruct() {bzero(this,sizeof(*this);}");
		///////////////////////////////////////////////////////////////////////////
		//convto
		WRITE_LINE("void\t\tconvto(%s & convtomsg_) const {", desc->name().c_str());
		++level;
		for (auto & sfm : msg_meta.sub_fields){
			//set
			string st_var_name = sfm.GetVarName();
			if (sfm.field_desc->is_repeated()){
				WRITE_LINE("assert(%s.count <= %s);//assertion", st_var_name.c_str(), sfm.count.c_str());
				//for(size_t i = 0;i < a.count; ++i)
				WRITE_LINE("for ( size_t i = 0; i < %s.count &&  && i < %s; ++i){", st_var_name.c_str(), sfm.count.c_str());
				st_var_name += ".list[i]";
				++level;
			}
			WRITE_LINE("%s;", sfm.GetScalarConvToMeth("convtomsg_", st_var_name, sfm.field_desc->name()).c_str());
			if (sfm.field_desc->is_repeated()){
				--level;
				WRITE_LINE("}");
			}
		}
		--level;
		WRITE_LINE("}");
		///////////////////////////////////////////////////////////////////////////
		//convto
		WRITE_LINE("void\t\tconvfrom(const %s & convfrommsg_) {", desc->name().c_str());
		++level;
		for (auto & sfm : msg_meta.sub_fields){
			//set
			string st_var_name = sfm.GetVarName();
			string msg_var_name = sfm.field_desc->name();
			if (sfm.field_desc->is_repeated()){
				WRITE_LINE("assert(convfrommsg_.%s_size() <= %s);//assertion", msg_var_name.c_str(), sfm.count.c_str());
				//for(size_t i = 0;i < a.count; ++i)
				WRITE_LINE("%s.count = 0;", sfm.GetVarName().c_str());
				WRITE_LINE("for ( int i = 0; i < convfrommsg_.%s_size() && i < %s; ++i){", msg_var_name.c_str(), sfm.count.c_str());
				st_var_name += ".list[i]";
				msg_var_name += "(i)";
				++level;
			}
			WRITE_LINE("%s;", sfm.GetScalarConvFromMeth("convfrommsg_", st_var_name, msg_var_name.c_str()).c_str());
			if (sfm.field_desc->is_repeated()){
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
			}
		}
		--level;
		WRITE_LINE("}");
		///////////////////////////////////////////////////////////////////////////////
		//bool operator ==  //for find
		WRITE_LINE("bool\t\toperator == (const %s & rhs_) {", STMetaUtil::GetStructName(desc).c_str());
		++level;
		for (size_t i = 0; i < msg_meta.pks_fields.size(); ++i){
			string pred_prefix = "";
			string pred_postfix = "";
			if (i == 0){
				pred_prefix = "return";
			}
			if (i + 1 == msg_meta.pks_fields.size()){
				pred_postfix = ";";
			}
			else {
				pred_postfix = "&&";
			}
			auto & sfm = *msg_meta.pks_fields[i];
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_FLOAT ||
				sfm.field_desc->type() == FieldDescriptor::TYPE_DOUBLE){
				WRITE_LINE("%s (fabs(%s - rhs_.%s) < 10e-6) %s",
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					pred_postfix.c_str());
			}
			else if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){					
				WRITE_LINE("%s (strncmp(%s.data, rhs.%s.data, %s) == 0) %s", 
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.length.c_str(),pred_postfix.c_str());
			}
			else if (sfm.field_desc->type() == FieldDescriptor::TYPE_BYTES){
				WRITE_LINE("%s (%s.length == rhs_.%s.length && memcmp(%s.data, rhs_.%s.data, %s.length) == 0) %s",
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.GetVarName().c_str(), pred_postfix.c_str());
			}
			else {
				WRITE_LINE("%s (%s == rhs_.%s) %s",
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					pred_postfix.c_str());
			}
			if (i == 0){
				++level;
			}
			if (i + 1 == msg_meta.pks_fields.size()){
				--level;
			}
		}
		--level;
		WRITE_LINE("}");

		//bool operator <  //for less //////////////////////////////////////////
		WRITE_LINE("bool\t\toperator < (const %s & rhs_) {", STMetaUtil::GetStructName(desc).c_str());
		++level;
		for (size_t i = 0; i < msg_meta.pks_fields.size(); ++i){
			string pred_prefix = "";
			string pred_postfix = "";
			if (i == 0){
				pred_prefix = "return";
			}
			if (i + 1 == msg_meta.pks_fields.size()){
				pred_postfix = ";";
			}
			else {
				pred_postfix = "&&";
			}
			auto & sfm = *msg_meta.pks_fields[i];
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("%s (strncmp(%s.data, rhs_.%s.data, %s) < 0 ) %s",
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.length.c_str(), pred_postfix.c_str());
			}
			else if (sfm.field_desc->type() == FieldDescriptor::TYPE_BYTES){
				WRITE_LINE("%s (memcmp(%s.data, rhs_.%s.data, std::min(%s.length, rhs_.%s.length) < 0 ) %s",
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					pred_postfix.c_str());
			}
			else {
				WRITE_LINE("%s (%s < rhs_.%s ) %s",
					pred_prefix.c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					pred_postfix.c_str());
			}
			if (i == 0){
				++level;
			}
			if (i + 1 == msg_meta.pks_fields.size()){
				--level;
			}
		}
		///////////
		--level;
		WRITE_LINE("}");

		//////////////////////////////////////////////////////////////////////////////////////
		for (auto & sfm : msg_meta.sub_fields){
			if (sfm.count.empty()){
				continue;
			}
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_BYTES){
				continue;
			}
			//----finx_idx_------------------------------------------------------
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tfind_idx_%s(const std::string & entry_) {",
						sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("auto it = std::find(%s.list, %s.list + %s.count, entry_,",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("[](const decltype(%s.list[0]) & t1, const std::string & t2){ return t2 > t1.data; }); ",
					sfm.GetVarName().c_str());
				--level;
			}
			else {
				WRITE_LINE("int\t\tfind_idx_%s(const %s & entry_) {", 
					sfm.GetVarName().c_str(), sfm.GetTypeName().c_str());
				++level;
				WRITE_LINE("auto it = std::find(%s.list, %s.list + %s.count, entry_);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
			}
			WRITE_LINE("if ( it != %s.list + %s.count ) { return it - %s.list; }",
				sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
			WRITE_LINE("return -1;");
			--level;
			WRITE_LINE("}");
			//-------append_ shift------------------------------------------------
			//int	append_x(entry & v , bool shift)
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tappend_%s(const std::string & entry_, bool shift_ = true) {",
					sfm.GetVarName().c_str());
				++level;
				//if entry_.length() >= count return -1
				WRITE_LINE("if ( entry_.length() >= %s ) { return -1; }", sfm.length.c_str());
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(),sfm.count.c_str());
				++level;
				//%s.list[count] = entry_;
				//++count;
				WRITE_LINE("strncpy(%s.list[%s.count].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),sfm.length.c_str());
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("else {");
				++level;
				//if shift_
				//<-
				//else return -1;
				//for(size_t i = 0 ;i < count; ++i){
				WRITE_LINE("if ( !shift_ ) { return -2; }");
				//memmove(src, src + 1, n - 1);
				//src[n - 1] = entry_;
				WRITE_LINE("memmove(%s.list, %s.list + 1, (%s-1)*sizeof(%s.list[0]));",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("strncpy(%s.list[%s - 1].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.count.c_str(), sfm.length.c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("return 0;");
				--level;
				WRITE_LINE("}");
			}
			else {
				WRITE_LINE("int\t\tappend_%s(const %s & entry_, bool shift_ = false) {",
					sfm.GetVarName().c_str(), sfm.GetScalarTypeName().c_str());
				++level;
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.count.c_str());
				++level;
				//%s.list[count] = entry_;
				//++count;
				WRITE_LINE("%s.list[%s.count] = entry_;",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("else {");
				++level;
				WRITE_LINE("if ( !shift_ ) { return -2; }");
				//memmove(src, src + 1, n - 1);
				//src[n - 1] = entry_;
				WRITE_LINE("memmove(%s.list, %s.list + 1, (%s-1)*sizeof(%s.list[0]));",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("%s.list[%s - 1] = entry_;",
					sfm.GetVarName().c_str(), sfm.count.c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("return 0;");
				--level;				
				WRITE_LINE("}");
			}
			//-------insert_ ------------------------------------------------
			//int	insert_x(int idx_, entry & v , bool shift_force_insert = false)
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tinsert_%s(int idx_, const std::string & entry_, bool shift_force_insert_ = true) {",
					sfm.GetVarName().c_str());
				++level;
				//if entry_.length() >= count return -1
				WRITE_LINE("if ( entry_.length() >= %s ) { return -1; }", sfm.length.c_str());
				WRITE_LINE("if ( idx_ < 0 || idx_ >= %s || idx_ > %s.count ) { return -2; }",
					sfm.count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.count.c_str());
				++level;
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s.count - idx_)*sizeof(%s.list[0]);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("strncpy(%s.list[idx_].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.length.c_str());
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("else {");
				++level;
				WRITE_LINE("if ( !shift_force_insert_ ) { return -3; }");
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s - 1 - idx_)*sizeof(%s.list[0]);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("strncpy(%s.list[idx_].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.length.c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("return 0;");
				--level;
				WRITE_LINE("}");
			}
			else {
				WRITE_LINE("int\t\tinsert_%s(int idx_, const %s & entry_, bool shift_ = false) {",
					sfm.GetVarName().c_str(), sfm.GetScalarTypeName().c_str());
				++level;
				WRITE_LINE("if ( idx_ < 0 || idx_ >= %s || idx_ > %s.count ) { return -2; }",
					sfm.count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.count.c_str());
				++level;
				//->
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s.count - idx_)*sizeof(%s.list[0]);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("%s.list[idx_] = entry_;",sfm.GetVarName().c_str());
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("else {");
				++level;
				WRITE_LINE("if ( !shift_force_insert_ ) { return -3; }");
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s - 1 - idx_)*sizeof(%s.list[0]);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("%s.list[idx_] = entry_;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("return 0;");
				--level;
				WRITE_LINE("}");
			}
			//-------remove_ x(swap_not_shift)------------------------------------------------
			//int	remove_x(int idx , bool shift)
			WRITE_LINE("int\t\tremove_%s(size_t idx_, bool shift_not_swap_ = true) {",
				sfm.GetVarName().c_str());
			++level;
			WRITE_LINE("if ( idx_ >= %s.count ) { return -1; }", sfm.GetVarName().c_str());
			WRITE_LINE("if ( shift_not_swap_ ) {");
			++level;
			//shift <- idx_
			WRITE_LINE("memmove(%s.list + idx_, %s.list + idx_ + 1, (%s.count - idx_ - 1)*sizeof(%s.list[0]));",
				sfm.GetVarName().c_str(),
				sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
				sfm.GetVarName().c_str());
			--level;
			WRITE_LINE("}");
			WRITE_LINE("else { %s.list[idx_] = %s.list[%s.count - 1]; }",
				sfm.GetVarName().c_str(),
				sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
			WRITE_LINE("--%s.count;", sfm.GetVarName().c_str());
			WRITE_LINE("return 0;");
			--level;
			WRITE_LINE("}");
			//-------lower_bound_ x()------------------------------------------------
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tlower_bound_%s(const std::string & entry_) {",
					sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("auto it = std::lower_bound(%s.list, %s.list + %s.count, entry_,",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("[](const decltype(%s.list[0]) & t1, const std::string & t2){ return t2 > t1.data; }); ",
					 sfm.GetVarName().c_str());
				--level;

				WRITE_LINE("if ( it != %s.list + %s.count ) { return it - %s.list; }",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("return -1;");
				--level;
				WRITE_LINE("}");
			}
			else {
				WRITE_LINE("int\t\tlower_bound_%s(const %s & entry_) {",
					sfm.GetVarName().c_str(), sfm.GetScalarTypeName().c_str());
				++level;
				WRITE_LINE("auto it = std::lower_bound(%s.list, %s.list + %s.count, entry_);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("if ( it != %s.list + %s.count ) { return it - %s.list; }",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("return -1;");
				--level;
				WRITE_LINE("}");
			}
			//-------upper_bound_ x(swap_not_shift)------------------------------------------------
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tupper_bound_%s(const std::string & entry_) {",
					sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("auto it = std::upper_bound(%s.list, %s.list + %s.count, entry_,",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("[](const decltype(%s.list[0]) & t1, const std::string & t2){ return t2 > t1.data; }); ",
					sfm.GetVarName().c_str());
				--level;

				WRITE_LINE("if ( it != %s.list + %s.count ) { return it - %s.list; }",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("return -1;");
				--level;
				WRITE_LINE("}");
			}
			else {
				WRITE_LINE("int\t\tupper_bound_%s(const %s & entry_) {",
					sfm.GetVarName().c_str(), sfm.GetScalarTypeName().c_str());
				++level;
				WRITE_LINE("auto it = std::upper_bound(%s.list, %s.list + %s.count, entry_);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("if ( it != %s.list + %s.count ) { return it - %s.list; }",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("return -1;");
				--level;
				WRITE_LINE("}");
			}
			//-------binsert_(swap_not_shift)------------------------------------------------
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tbinsert_%s(const std::string & entry_, bool shift_force_insert_ = false) {",
					sfm.GetVarName().c_str());
			}
			else {
				WRITE_LINE("int\t\tbinsert_%s(const %s & entry_, bool shift_force_insert_ = false) {",
					sfm.GetVarName().c_str(), sfm.GetScalarTypeName().c_str());
			}
			++level;
			//insert after
			//int idx_ = upper_bound_x();
			//if (idx_ < 0) { append());
			//else if (insert());
			WRITE_LINE("int idx_ = upper_bound_%s(entry_);", sfm.GetVarName().c_str());
			WRITE_LINE("if ( idx_ < 0 ) { return append_%s(entry_, shift_force_insert_); }", sfm.GetVarName().c_str());
			WRITE_LINE("return insert_%s(idx_, entry_, shift_force_insert_);", sfm.GetVarName().c_str());
			--level;
			WRITE_LINE("}");
		}

		///////////////////////////////////////////////////////////////////////////////
		level--;
		WRITE_LINE("};");
		return ss.str();
	}
	void	print(){
		TopologySort(ordered_desc);
		for (int i = 0; i < (int)ordered_desc.size(); ++i){
			cout << ConvertMsg(ordered_desc[i]);
		}
	}
	void	TopologySort(std::vector<const Descriptor * > & msgs){
		msgs.clear();
		std::queue<const Descriptor *>  desc_queue;
		desc_queue.push(root);
		msg_degrees[root] = 0;
		while (!desc_queue.empty()){
			auto msg = desc_queue.front();
			desc_queue.pop();
			for (int i = 0; i < msg->field_count(); ++i){
				auto field_def = msg->field(i);
				if (field_def->type() == FieldDescriptor::TYPE_MESSAGE){
					auto field = field_def->message_type();
					if (reverse_refer[field].empty()){
						desc_queue.push(field);
						msg_degrees[field] = 0;
					}
					int md = reverse_refer[field].size();
					reverse_refer[field].insert(msg);
					if (md != reverse_refer[field].size()){
						++msg_degrees[msg];//out degree
					}
				}
			}
		}
		/////////////////////////////////////////////////////////
		while (!msg_degrees.empty()){	
			auto it = msg_degrees.begin();
			while (it != msg_degrees.end()){
				if (it->second == 0){
					break;
				}
				++it;
			}
			if (it == msg_degrees.end()){
				error_stream << "msg degree not found zero !" << endl;
				throw logic_error("msg degree error !");
			}
			msgs.push_back(it->first);
			for (auto desc : reverse_refer[it->first]){
				if (msg_degrees[desc] > 0){
					--msg_degrees[desc];
				}
				else {
					error_stream << "msg type:" << desc->name() << " refer count is 0 !" << endl;
				}
			}
			msg_degrees.erase(it);
		}
	}
};
void usage(){

}

int main(int argc, char * argv[]){
	if (argc < 3){
		cout << "usage:" << argv[0] << " <file name> <root message type> [path]" << endl;
		return -1;
	}
	compiler::DiskSourceTree dst;
	dst.MapPath("", "/usr/local/include");
	dst.MapPath("", "/usr/include");
	if (argc > 3){
		dst.MapPath("", argv[3]);
	}
	dst.MapPath("", ".");
	MFEC mfec;
	compiler::Importer	importer(&dst,&mfec);
	auto ret = importer.Import(argv[1]);
	if (!ret){
		cerr << "error import !" << endl;
		return -1;
	}
	auto pool = importer.pool();
	auto desc = pool->FindMessageTypeByName(argv[2]);
	GenerateCXXFlat gen(desc);
	try {
		gen.print();
	}
	catch (...){
		cerr << "generate code error ! for:" << error_stream.str() << endl;
		return -2;
	}
	return 0;
}