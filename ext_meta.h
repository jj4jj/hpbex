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
#include<stdarg.h>
namespace google {
	namespace protobuf {
		class Descriptor;
		class FieldDescriptor;
		class FileDescriptor;
		class DescriptorPool;
		class Message;
		class EnumValueDescriptor;
		class EnumDescriptor;

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
	static int32_t			GetEnumValue(const char * name, DESC desc_){
		if (!name || !name[0]){
			return 0;
		}
		if (name[0] >= 0 && name[0] <= 9){
			return atoi(name);
		}
		auto f_desc = desc_->file();
		auto ev_desc = f_desc->FindEnumValueByName(name);
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
	void	ParseSubFields();
	int		ParsePKS();
};



class EXTProtoMeta {
	google::protobuf::compiler::DiskSourceTree * dst;
	google::protobuf::compiler::Importer *		 importer;
public:
	EXTProtoMeta();
	~EXTProtoMeta();
	int		Init(const char * path, ...);
	const google::protobuf::FileDescriptor * LoadFile(const char * file);
	const google::protobuf::DescriptorPool * GetPool();

};