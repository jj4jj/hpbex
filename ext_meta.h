#pragma  once
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
#include <cstdarg>
namespace google {
	namespace protobuf {
		class Descriptor;
		class FieldDescriptor;
		class FileDescriptor;
		class Reflection;
		class DescriptorPool;
		class Message;
		class EnumValueDescriptor;
		class EnumDescriptor;
		class DynamicMessageFactory;
		namespace compiler {
			class DiskSourceTree;
			class Importer;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////
struct EXTEnumValueMeta {
	const google::protobuf::EnumValueDescriptor * ev_desc;
	std::string e_cn;
	std::string e_desc;
	int			AttachDesc(const google::protobuf::EnumValueDescriptor * fd);
};

//////////////////////////////////////////////////////////////////////////////////////////////
struct EXTEnumMeta {
	const google::protobuf::EnumDescriptor * ev_desc;
};


//////////////////////////////////////////////////////////////////////////////////////////////
struct EXTFieldMeta {
	const google::protobuf::FieldDescriptor * field_desc;
	std::string f_count;
	std::string f_length;
	std::string f_cn;
	std::string f_desc;
	//////////////////////////////////
	int32_t		z_length;
	int32_t		z_count;

	int			AttachDesc(const google::protobuf::FieldDescriptor * fd);
	std::string GetScalarTypeName();
	std::string GetTypeName();
	std::string GetVarName();
	std::string GetScalarConvToMeth(const char * convtomsg_, const std::string & st_var_name, const std::string & msg_var_name);
	std::string GetScalarConvFromMeth(const char * convtomsg_, const std::string & st_var_name, const std::string & msg_var_name);
	std::string GetMysqlFieldType();
};

struct EXTMetaUtil {
	static std::string		GetStructName(const google::protobuf::Descriptor * desc);
	template<class DESC>
	static int32_t			GetEnumValue(const char * name_or_number, DESC desc_){
		if (!name_or_number || !name_or_number[0]){
			return 0;
		}
		if (name_or_number[0] >= '0' && name_or_number[0] <= '9'){
			return atoi(name_or_number);
		}
		auto f_desc = desc_->file();
		auto ev_desc = f_desc->FindEnumValueByName(name_or_number);
		if (ev_desc){
			return ev_desc->number();
		}
		return 0;
	}
};

struct EXTMessageMeta {
	const google::protobuf::Descriptor * msg_desc;
	std::string m_pks;
	std::string m_divkey;
	int32_t		m_divnum;
	std::string m_cn;
	std::string m_desc;
	std::string m_relchk;
	std::string m_autoinc;
	std::vector<EXTFieldMeta*>		pks_fields;//if no , then all
	std::vector<EXTFieldMeta>		sub_fields;
	std::vector<std::string>		pks_name;
public:
	int	    AttachDesc(const google::protobuf::Descriptor * desc);
	void	construct();
private:
	int     ParseSubFields();
	int		ParsePKS();
};

class EXTProtoMeta {
	google::protobuf::compiler::DiskSourceTree * dst;
	google::protobuf::compiler::Importer *		 importer;
	google::protobuf::DynamicMessageFactory	*	 dyn_msg_factory;
public:
	EXTProtoMeta();
	~EXTProtoMeta();
    int		Init(const char ** paths, int n = 0, const char ** otherfiles = nullptr, int m = 0);
	const google::protobuf::FileDescriptor * LoadFile(const char * file);
	const google::protobuf::DescriptorPool * GetPool();
	const google::protobuf::Descriptor *	 GetMsgDesc(const char * msg_type);
	google::protobuf::Message	*			 NewDynMessage(const char * msg_type, const char * buffer = 0, size_t buffer_len = 0);	
	void									 FreeDynMessage(google::protobuf::Message * msg);

};
