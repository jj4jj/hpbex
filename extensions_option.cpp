
#include "extensions.pb.h"
#include "google/protobuf/descriptor.h"
#include "extensions_option.h"
#include <string>
#include <iostream>
using namespace std;
using namespace google::protobuf;

#define CHECK_STR_OPT(chkname)	\
if (strcmp(name_, #chkname) == 0){\
	string chkname_val = proto_desc->options().GetExtension(chkname); \
	return chkname_val; \
}

#define CHECK_INT_OPT(chkname, chktype)	\
if (strcmp(name_, #chkname) == 0){\
	chktype chkname_val = proto_desc->options().GetExtension(chkname); \
	return to_string(chkname_val); \
}

string proto_msg_opt(const Descriptor * proto_desc, const char * name_){
	CHECK_STR_OPT(m_pks);
	CHECK_STR_OPT(m_divkey);
	CHECK_INT_OPT(m_divnum, uint32_t);
	CHECK_STR_OPT(m_cn);
	CHECK_STR_OPT(m_desc);
	CHECK_STR_OPT(m_relchk);
	CHECK_STR_OPT(m_autoinc);
	return "";
}
string proto_field_opt(const FieldDescriptor * proto_desc, const char * name_){
	CHECK_STR_OPT(f_cn);
	CHECK_STR_OPT(f_count);
	CHECK_STR_OPT(f_desc);
	CHECK_STR_OPT(f_length);
	return "";
}
string proto_enum_opt(const EnumValueDescriptor * proto_desc, const char * name_){
	CHECK_STR_OPT(e_cn);
	CHECK_STR_OPT(e_desc);
	return "";
}
