#pragma  once
#include <string>

namespace google {
	namespace protobuf {
		class Descriptor;
		class FieldDescriptor;
		class EnumValueDescriptor;
	}
}
/*
optional string m_pks = 50000;
optional string m_divkey = 50001;
optional uint32 m_divnum = 50002;
optional string m_cn = 50003;
optional string m_desc = 50004;
optional string m_relchk = 50005;
optional string m_autoinc = 50006;
}
extend google.protobuf.FieldOptions {
optional string  f_cn = 50000;
optional string  f_desc = 50001;
optional string  f_count = 50002;
optional string  f_length = 50003;
}
extend google.protobuf.EnumValueOptions {
optional string e_cn = 50000;
optional string e_desc = 50001;
*/

std::string proto_msg_opt(const google::protobuf::Descriptor * desc, const char * name);
std::string proto_field_opt(const google::protobuf::FieldDescriptor * desc, const char * name);
std::string proto_enum_opt(const google::protobuf::EnumValueDescriptor * desc, const char * name);
