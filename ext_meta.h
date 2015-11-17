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

		namespace compiler {
			class DiskSourceTree;
			class Importer;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////
struct STFieldMeta {
	const google::protobuf::FieldDescriptor * field_desc;
	std::string f_count;
	std::string f_length;
	std::string f_cn;
	std::string f_desc;

	int			ParseFrom(const google::protobuf::FieldDescriptor * fd);
	std::string GetScalarTypeName();
	std::string GetTypeName();
	std::string GetVarName();
	std::string GetScalarConvToMeth(const char * convtomsg_, const std::string & st_var_name, const std::string & msg_var_name);
	std::string GetScalarConvFromMeth(const char * convtomsg_, const std::string & st_var_name, const std::string & msg_var_name);

};


struct STMessageMetaUtil {
	static std::string GetStructName(const google::protobuf::Descriptor * desc);
};

struct STMessageMeta {
	const google::protobuf::Descriptor * msg_desc;
	std::string m_pks;
	std::string m_divkey;
	int32_t		m_divnum;
	std::string m_cn;
	std::string m_desc;
	std::string m_relchk;
	std::string m_autoinc;
	std::vector<STFieldMeta*>		pks_fields;//if no , then all
	std::vector<STFieldMeta>		sub_fields;
	std::vector<std::string>		pks_name;
	int	    ParseFrom(const google::protobuf::Descriptor * desc);
	void	ParseSubFields();
	int		ParsePKS();
};



class ProtoMeta {
	google::protobuf::compiler::DiskSourceTree * dst;
	google::protobuf::compiler::Importer *		 importer;
public:
	ProtoMeta();
	~ProtoMeta();
	int		Init(const char * path, ...);
	const google::protobuf::FileDescriptor * LoadFile(const char * file);
	const google::protobuf::DescriptorPool * GetPool();

};