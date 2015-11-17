#include "ext_meta.h"
#include "google/protobuf/compiler/importer.h"
#include "extensions_option.h"

std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;



#define DIM_ARRAY(a)	(sizeof((a))/sizeof((a)[0]))

template<class D>
static inline string descriptor_option(D desc, const string & opt);
template<>
inline string descriptor_option(const Descriptor * desc, const string & opt){
	return proto_msg_opt(desc, opt.c_str());
}
template<>
inline string descriptor_option(const FieldDescriptor * desc, const string & opt){
	return proto_field_opt(desc, opt.c_str());
}
template<>
inline string descriptor_option(const EnumValueDescriptor * desc, const string & opt){
	return proto_enum_opt(desc, opt.c_str());
}


template<class D>
int GetDescOption(string & value,D desc, const string & option){
	value = descriptor_option(desc, option);
	if (!value.empty()) {
		return 0;
	}
	error_stream << "not found the option:(" << option.c_str() << ") in desc:" << desc->name() << endl;
	return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////
#define GET_DESC_STR_OPTION(opt_name, desc)	GetDescOption(opt_name, desc, #opt_name)
#define GET_DESC_INT_OPTION(opt_name, desc)	do{string _value_opt_str;GetDescOption(_value_opt_str, desc, #opt_name);opt_name = std::stoi(_value_opt_str);}while(false)
	

int	    STFieldMeta::ParseFrom(const FieldDescriptor * desc){
	field_desc = desc;

	GET_DESC_STR_OPTION(f_cn, field_desc);
	GET_DESC_STR_OPTION(f_desc, field_desc);
	GET_DESC_STR_OPTION(f_count, field_desc);
	GET_DESC_STR_OPTION(f_length, field_desc);

	if (desc->is_repeated()){
		//cout > 0
		if (f_count.empty()){
			error_stream << "the field:" << desc->name() << " in message:" << desc->type_name() << " is a repeat , but not found options [count] !" << endl;
			return -1;
		}
	}
	if (desc->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
		if (f_length.empty()){
			error_stream << "the field:" << desc->name() << " in message:" << desc->type_name() << " is a string , but not found options [length] !" << endl;
			return -2;
		}
	}
	return 0;
}
string STFieldMeta::GetScalarTypeName(){
	static const char * s_var_type_names[] = { "", "int32_t", "uint32_t", "int64_t", "uint64_t", "double", "float", "const char *", "int32_t", "Message" };
	const char * pszTypeName = s_var_type_names[field_desc->cpp_type()];
	if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_ENUM){
		return field_desc->enum_type()->name();
	}
	else if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
		return STMessageMetaUtil::GetStructName(field_desc->message_type());
	}
	return pszTypeName;
}
string STFieldMeta::GetTypeName() {
	const char * pszTypeName = GetScalarTypeName().c_str();
	if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
		if (field_desc->type() == FieldDescriptor::TYPE_STRING){
			string buffer_type_name = "struct { char data[";
			buffer_type_name += f_length;
			buffer_type_name += "]; }";
			return buffer_type_name;
		}
		else {
			string buffer_type_name = "struct { size_t length; uint8_t data[";
			buffer_type_name += f_length;
			buffer_type_name += "]; }";
			return buffer_type_name;
		}
	}
	else {
		return pszTypeName;
	}
}
string STFieldMeta::GetVarName() {
	//static const char * type_prefix = ["", "i", "ll", "dw", "ull", "df", "f", "b", "en", "str", "st"];
	//desc->camelcase_name();
	return field_desc->lowercase_name();
}
string STFieldMeta::GetScalarConvToMeth(const char * convtomsg_, const string & st_var_name, const string & msg_var_name){
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
string STFieldMeta::GetScalarConvFromMeth(const char * convtomsg_, const string & st_var_name, const string & msg_var_name){
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
			meth += f_length.c_str();
			meth += "-1)";
		}
		else if (field_desc->type() == FieldDescriptor::TYPE_BYTES){
			meth = "memcpy(";
			meth += st_var_name.c_str();
			meth += ".data, ";
			meth += convtomsg_;
			meth += ".";
			meth += msg_var_name;
			meth += ".data(), std::min((size_t)";
			meth += f_length.c_str();
			meth += ", (size_t)";
			meth += convtomsg_;
			meth += ".";
			meth += msg_var_name + ".length()))";
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

//////////////////////////////////////////////////////////////////////////
std::string STMessageMetaUtil::GetStructName(const google::protobuf::Descriptor * desc){
	std::string sname = desc->name();
	sname += "_ST";
	return sname;
}

//////////////////////////////////////////////////////////////////////////
int	    STMessageMeta::ParseFrom(const Descriptor * desc){
	msg_desc = desc;

	GET_DESC_STR_OPTION(m_pks, msg_desc);
	GET_DESC_STR_OPTION(m_divkey, msg_desc);
	GET_DESC_INT_OPTION(m_divnum, msg_desc);
	GET_DESC_STR_OPTION(m_cn, msg_desc);
	GET_DESC_STR_OPTION(m_desc, msg_desc);
	GET_DESC_STR_OPTION(m_relchk, msg_desc);
	GET_DESC_STR_OPTION(m_autoinc, msg_desc);

	////////////////////////////
	ParseSubFields();
	int ret = ParsePKS();

	return ret;
}
void	STMessageMeta::ParseSubFields(){
	for (int i = 0; i < msg_desc->field_count(); ++i){
		STFieldMeta sfm;
		sfm.ParseFrom(msg_desc->field(i));
		sub_fields.push_back(sfm);
	}
}
int		STMessageMeta::ParsePKS(){
	string::size_type bpos = 0, fpos = 0;
	while (true && !m_pks.empty()){
		fpos = m_pks.find(',', bpos);
		string f_field_name = "";
		if (fpos != string::npos && fpos > bpos){
			f_field_name = m_pks.substr(bpos, fpos - bpos);
		}
		else {
			if (fpos != bpos){
				f_field_name = m_pks.substr(bpos);
			}
		}
		pks_name.push_back(f_field_name);
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

/////////////////////////////////////////////////////////////////////////////////

struct ProtoMetaErrorCollector : google::protobuf::compiler::MultiFileErrorCollector {
	void AddError(
	const string & filename,
	int line,
	int column,
	const string & message)
	{
		error_stream << "file name:" << filename << ":" << line << " error:" << message << endl;
	}
};

ProtoMeta::ProtoMeta(){
	dst = new google::protobuf::compiler::DiskSourceTree();
	dst->MapPath("", "/usr/local/include");
	dst->MapPath("", "/usr/include");
	dst->MapPath("", ".");
	importer = nullptr;
}
ProtoMeta::~ProtoMeta(){
	if (importer){
		delete importer;
	}
	if (dst){
		delete dst;
	}
}
int		ProtoMeta::Init(const char * path, ...){
	if (importer){
		return -1;
	}
	va_list arg;
	va_start(arg, path);
	while (path){
		dst->MapPath("", path);
		path = va_arg(arg, const char *);
	}
	va_end(arg);
	ProtoMetaErrorCollector mfec;
	importer = new google::protobuf::compiler::Importer(dst, &mfec);
	return 0;
}
const google::protobuf::FileDescriptor * ProtoMeta::LoadFile(const char * file){
	auto ret = importer->Import(file);
	if (!ret){
		cerr << "error import file:" << file << endl;
		return nullptr;
	}
	return ret;
}
const google::protobuf::DescriptorPool * ProtoMeta::GetPool(){
	return importer->pool();
}
