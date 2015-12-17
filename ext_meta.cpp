#include "ext_meta.h"
#include "google/protobuf/compiler/importer.h"
#include "google/protobuf/dynamic_message.h"
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
	//error_stream << "info: not found the option:(" << option.c_str() << ") in desc:" << desc->name() << endl;
	return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////
#define GET_DESC_STR_OPTION(opt_name, desc)	GetDescOption(opt_name, desc, #opt_name)
#define GET_DESC_INT_OPTION(opt_name, desc)	do{string _value_opt_str;GetDescOption(_value_opt_str, desc, #opt_name);opt_name = std::stoi(_value_opt_str);}while(false)
	
///////////////////////////////////////////////////////////////////////////////////////////////
int	    EXTEnumValueMeta::AttachDesc(const EnumValueDescriptor * desc){
	ev_desc = desc;
	GET_DESC_STR_OPTION(e_cn, ev_desc);
	GET_DESC_STR_OPTION(e_desc, ev_desc);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
int	    EXTFieldMeta::AttachDesc(const FieldDescriptor * desc){
	field_desc = desc;

	GET_DESC_STR_OPTION(f_cn, field_desc);
	GET_DESC_STR_OPTION(f_desc, field_desc);
	GET_DESC_STR_OPTION(f_count, field_desc);
	GET_DESC_STR_OPTION(f_length, field_desc);

	z_length = EXTMetaUtil::GetEnumValue(f_length.c_str(), field_desc);
	z_count = EXTMetaUtil::GetEnumValue(f_count.c_str(), field_desc);
	if (desc->is_repeated()){
		//cout > 0
		if (z_count <= 0){
			error_stream << "the field: <" << desc->name() << "> is a repeated  , but not found options [f_count:"<< f_count <<"] or count is a error num value:" << z_count << endl;
			return -1;
		}
	}
	if (desc->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
		if (z_length <= 0){
			error_stream << "the field: <" << desc->name() << "> is a string , but not found options [f_length:" << f_length << "] or length is a error num value:" << z_length << endl;
			return -2;
		}
	}
	return 0;
}
string EXTFieldMeta::GetScalarTypeName(){
	static const char * s_var_type_names[] = { "",
		"int32_t", "int64_t", "uint32_t",
		"uint64_t", "double", "float",
		"bool", "invalidate type(enum)", "invalidate type(const char *)",
		"invalidate type(message)","" };
	const char * pszTypeName = s_var_type_names[field_desc->cpp_type()];
	if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_ENUM){
		return field_desc->enum_type()->name();
	}
	else if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
		return EXTMetaUtil::GetStructName(field_desc->message_type());
	}
	return pszTypeName;
}
string EXTFieldMeta::GetTypeName() {
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
string EXTFieldMeta::GetVarName() {
	//static const char * type_prefix = ["", "i", "ll", "dw", "ull", "df", "f", "b", "en", "str", "st"];
	//desc->camelcase_name();
	string lc_name = field_desc->lowercase_name();
	return lc_name;
}
string EXTFieldMeta::GetScalarConvToMeth(const char * convtomsg_, const string & st_var_name, const string & msg_var_name){
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
string EXTFieldMeta::GetScalarConvFromMeth(const char * convtomsg_, const string & st_var_name, const string & msg_var_name){
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
std::string EXTFieldMeta::GetMysqlFieldType(){
	switch (field_desc->cpp_type()){
	case FieldDescriptor::CPPTYPE_INT32:// 1,     // TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
	case FieldDescriptor::CPPTYPE_ENUM:// 8,     // TYPE_ENUM
		return "INT";
	case FieldDescriptor::CPPTYPE_UINT32:// 3,     // TYPE_UINT32, TYPE_FIXED32
		return "INT UNSIGNED";
	case FieldDescriptor::CPPTYPE_INT64:// 2,     // TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
		return "BIGINT";
	case FieldDescriptor::CPPTYPE_UINT64:// 4,     // TYPE_UINT64, TYPE_FIXED64
		return "BIGINT UNSIGNED";
	case FieldDescriptor::CPPTYPE_DOUBLE:// 5,     // TYPE_DOUBLE
		return "DOUBLE";
	case FieldDescriptor::CPPTYPE_FLOAT:// 6,     // TYPE_FLOAT
		return "FLOAT";
	case FieldDescriptor::CPPTYPE_BOOL:// 7,     // TYPE_BOOL
		return "TINYINT";
	case FieldDescriptor::CPPTYPE_STRING:// 9,     // TYPE_STRING, TYPE_BYTES
		if (z_length <= 0XFF){
			return "VARCHAR(255)"; //255
		}
		else if (z_length <= 0XFFFF){
			return "TEXT";//64K
		}
		else if (z_length <= 0XFFFFFF){
			return "MEDIUMTEXT"; //16MB
		}
		else {
			return "LONGTEXT";//4GB
		}
	case FieldDescriptor::CPPTYPE_MESSAGE:// 10,    // TYPE_MESSAGE, TYPE_GROUP	}
		return "MEDIUMBLOB";	//16MB
		/*
		if (zSize <= 0XFFFF){
			return "BLOB";	//64K
		}
		else if(zSize <= 0XFFFFFF) {
			return "MEDIUMBLOB";	//16MB
		}
		else {
			return "LOGNGBLOB"; //4GB
		}
		*/
	default:
		return "MEDIUMBLOB";
	}
}

//////////////////////////////////////////////////////////////////////////
std::string EXTMetaUtil::GetStructName(const google::protobuf::Descriptor * desc){
	std::string sname = desc->name();
	sname += "_ST";
	return sname;
}

//////////////////////////////////////////////////////////////////////////
void	EXTMessageMeta::construct(){
	m_divnum = 0;
	pks_fields.clear();
	sub_fields.clear();
	pks_name.clear();
}

int	    EXTMessageMeta::AttachDesc(const Descriptor * desc){
	msg_desc = desc;

	GET_DESC_STR_OPTION(m_pks, msg_desc);
	GET_DESC_STR_OPTION(m_divkey, msg_desc);
	GET_DESC_INT_OPTION(m_divnum, msg_desc);
	GET_DESC_STR_OPTION(m_cn, msg_desc);
	GET_DESC_STR_OPTION(m_desc, msg_desc);
	GET_DESC_STR_OPTION(m_relchk, msg_desc);
	GET_DESC_STR_OPTION(m_autoinc, msg_desc);

	////////////////////////////
	int ret = ParseSubFields();
    if(ret){
        return ret;
    }
	ret = ParsePKS();

	return ret;
}
int     EXTMessageMeta::ParseSubFields(){
    int ret = 0;
	for (int i = 0; i < msg_desc->field_count(); ++i){
		EXTFieldMeta sfm;
		ret = sfm.AttachDesc(msg_desc->field(i));
        if(ret){
			error_stream << "parse field errro in message :" << msg_desc->full_name() << endl;
            return ret;
        }
		sub_fields.push_back(sfm);
	}
    return ret;
}
int		EXTMessageMeta::ParsePKS(){
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

EXTProtoMeta::EXTProtoMeta(){
	dst = new google::protobuf::compiler::DiskSourceTree();
	dst->MapPath("", "/usr/local/include");
	dst->MapPath("", "/usr/include");
	dst->MapPath("", ".");
	importer = nullptr;
	dyn_msg_factory = nullptr;
}
EXTProtoMeta::~EXTProtoMeta(){
	if (importer){
		delete importer;
	}
	if (dst){
		delete dst;
	}
	if (dyn_msg_factory){
		delete dyn_msg_factory;
	}
}
int		EXTProtoMeta::Init(const char * * path, int n, const char ** otherfiles , int m ){
	if (importer){
		return -1;
	}
	while (path && n-- > 0){
		dst->MapPath("", path[n]);
		//std::clog << "add path:" << path[n] << endl;
	}
	ProtoMetaErrorCollector mfec;
	importer = new google::protobuf::compiler::Importer(dst, &mfec);
    while (otherfiles && m-- > 0){
        importer->AddUnusedImportTrackFile(otherfiles[m]);
    }
	return 0;
}
const google::protobuf::FileDescriptor * EXTProtoMeta::LoadFile(const char * file){
	auto ret = importer->Import(file);
	if (!ret){
		cerr << "error import file:" << file << endl;
		return nullptr;
	}
	dyn_msg_factory = new DynamicMessageFactory(importer->pool());
	return ret;
}
const google::protobuf::DescriptorPool * EXTProtoMeta::GetPool(){
	return importer->pool();
}
const google::protobuf::Descriptor *	 EXTProtoMeta::GetMsgDesc(const char* msg_type){
	return importer->pool()->FindMessageTypeByName(msg_type);
}
google::protobuf::Message	*			 EXTProtoMeta::NewDynMessage(const char * msg_type, const char * buffer, size_t buffer_len){
	auto msg_desc = GetMsgDesc(msg_type);
	if (!msg_desc){
		cerr << "get msg desc error ! msg_type:" << msg_type << endl;
		return nullptr;
	}
	auto pProtMsg = dyn_msg_factory->GetPrototype(msg_desc);
	if (!pProtMsg){
		cerr << "get proto msg error ! msg_type:" << msg_type << endl;
		return nullptr;
	}
	Message * pMsg = pProtMsg->New();
	if (!pMsg){
		cerr << "new msg from proto error ! msg_type:" << msg_type << endl;
		return nullptr;
	}
	if (buffer_len == 0 || !buffer){
		return pMsg;
	}
	if (!pMsg->ParseFromArray(buffer, (int)buffer_len)){
		cerr << "unpack msg error !" << msg_type << endl;
		FreeDynMessage(pMsg);
		return nullptr;
	}
	return pMsg;
}
void									 EXTProtoMeta::FreeDynMessage(google::protobuf::Message * pMsg){
	delete pMsg;
}
