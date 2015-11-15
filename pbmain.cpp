
#include "google/protobuf/compiler/importer.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <queue>
#include <vector>
#include <sstream>
#include <string>

using namespace std;

using namespace google::protobuf;
struct MFEC : compiler::MultiFileErrorCollector {
	void AddError(
				const string & filename,
				int line,
				int column,
				const string & message)
	{
		cerr << "file name:" << filename << ":" << line << " error:" << message << endl;
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
	for (int i = 0; i < DIM_ARRAY(option_map_tag); ++i){
		if (strcmp(option.c_str(), option_map_tag[i].option) == 0){
			tag = option_map_tag[i].tag;
		}
	}
	if (!tag){
		cerr << "not found the option->tag map option:"<<option.c_str() << " in desc:" << desc->name() << endl;
		return -1;
	}
	auto options = desc->options();
	for (int j = 0; j < options.unknown_fields().field_count(); ++j){
		auto ouf = options.unknown_fields().field(j);
		if (ouf.number() == tag){ //cn
			value = ouf.length_delimited();
			return 0;
		}
	}
	cerr << "not found the option:" << option.c_str() << " tag:"<< tag << " in desc:" << desc->name() << endl;
	return -2;
}

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
	static string GetStructName(const Descriptor * desc){
		return desc->name();
	}
	string ConvertMsg(const Descriptor * desc, const char * indent = "    "){
		/*
	struct Msg {
	member var
	construct
	pack
	unpack
	list helper [opt]
	}
	*/
		//construct
		//pack
		//unpack
		//list
		//find
		//append
		//shift_append
		//remove
		//lowwer_bound
		//upper_bound
		//insert
		//var mem
		stringstream ss;
		int level = 0;
		char sz_line_buffer[1024];
		#define	WRITE_LINE(format, ...)		do{\
			snprintf(sz_line_buffer, sizeof(sz_line_buffer), format, ##__VA_ARGS__); \
			ss << GenerateCXXFlat::repeat(indent, level) << sz_line_buffer << std::endl; \
				}while (false)
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
						cerr << "the field:" << fd->name() << " in message:" << fd->type_name() << " is a repeat , but not found options [count] !" << endl;
						return -1;
					}
				}
				if (fd->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
					if (GetDescOption(length, fd, "length")){
						cerr << "the field:" << fd->name() << " in message:" << fd->type_name() << " is a string , but not found options [length] !" << endl;
						return -2;
					}
				}
				return 0;
			}
			string GetTypeName() {
				static const char * s_var_type_names[] = { "", "int32_t", "uint32_t", "int64_t", "uint64_t", "double", "float", "", "", "" };
				const char * pszTypeName = s_var_type_names[field_desc->cpp_type()];
				if (field_desc->cpp_type() == FieldDescriptor::CPPTYPE_ENUM){
					return field_desc->enum_type()->name();
				}
				else if(field_desc->cpp_type() == FieldDescriptor::CPPTYPE_STRING){
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
				else if(field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
					return GenerateCXXFlat::GetStructName(field_desc->message_type());
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
			string GetScalarConvToMeth(const char * convtomsg_,const string & st_var_name,const string & msg_var_name){
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
						meth += "=";
						meth += convtomsg_;
						meth += ".";
						meth += msg_var_name;
					}
				}
				return meth;
			}


		};
		//struct Msg {
		WRITE_LINE("struct %s {", GetStructName(desc).c_str());
		level++;
		std::vector<STFieldMeta>	fieldmeta_list;
		for (int i = 0; i < desc->field_count(); ++i){
			STFieldMeta sfm;
			auto field_desc = desc->field(i);
			if (sfm.ParseFrom(field_desc)){
				exit(-1);
			}
			if (sfm.count.empty()){
				WRITE_LINE("%s\t\t%s;", sfm.GetTypeName().c_str(), sfm.GetVarName().c_str());
			}
			else {
				WRITE_LINE("struct {size_t count; %s list[%s];}\t\t%s;",
					sfm.GetTypeName().c_str(), sfm.count.c_str(), sfm.GetVarName().c_str());
			}
			fieldmeta_list.push_back(sfm);
		}
		//construct
		WRITE_LINE("void\tconstruct() {bzero(this,sizeof(*this);}");

		//convto
		WRITE_LINE("int\tconvto(%s & convtomsg_) const {", desc->name().c_str());
		++level;
		WRITE_LINE("int ret = 0;");
		for (auto & sfm : fieldmeta_list){
			//set
			string st_var_name = sfm.GetVarName();
			if (sfm.field_desc->is_repeated()){
				auto fmt = sfm.field_desc->message_type();
				//for(size_t i = 0;i < a.count; ++i)
				WRITE_LINE("for(size_t i = 0;i < %s.count && i < %s; ++i){", st_var_name.c_str(), sfm.count.c_str());
				st_var_name += ".list[i]";
				++level;
			}
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_MESSAGE){
				WRITE_LINE("ret = %s;", sfm.GetScalarConvToMeth("convtomsg_", st_var_name, sfm.field_desc->name()).c_str());
				WRITE_LINE("if(!ret) {return ret;}");
			}
			else {
				WRITE_LINE("%s;", sfm.GetScalarConvToMeth("convtomsg_", st_var_name, sfm.field_desc->name()).c_str());
			}
			if (sfm.field_desc->is_repeated()){
				--level;
				WRITE_LINE("}");
			}
		}
		WRITE_LINE("return ret;");
		--level;
		WRITE_LINE("}");
		///////////////////////////////////////////////////////////////////////////
		//convto
		WRITE_LINE("int\tconvfrom(const %s & convfrommsg_) {", desc->name().c_str());
		++level;
		WRITE_LINE("int ret = 0;");
		for (auto & sfm : fieldmeta_list){
			//set
			string st_var_name = sfm.GetVarName();
			string msg_var_name = sfm.field_desc->name();
			if (sfm.field_desc->is_repeated()){
				//for(size_t i = 0;i < a.count; ++i)
				WRITE_LINE("%s.count=0;", sfm.GetVarName().c_str());
				WRITE_LINE("for(int i = 0;i < convfrommsg_.%s_size() && i < %s ; ++i){", msg_var_name.c_str(), sfm.count.c_str());
				st_var_name += ".list[i]";
				msg_var_name += "(i)";
				++level;
			}
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_MESSAGE){
				WRITE_LINE("ret = %s;", sfm.GetScalarConvFromMeth("convfrommsg_", st_var_name, msg_var_name.c_str()).c_str());
				WRITE_LINE("if(!ret) {return ret;}");
			}
			else {
				WRITE_LINE("%s;", sfm.GetScalarConvFromMeth("convfrommsg_", st_var_name, msg_var_name.c_str()).c_str());
			}
			if (sfm.field_desc->is_repeated()){
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
			}
		}
		WRITE_LINE("return ret;");
		--level;
		WRITE_LINE("}");


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
				cerr << "msg degree not found zero !" << endl;
				exit(-1);
			}
			msgs.push_back(it->first);
			for (auto desc : reverse_refer[it->first]){
				if (msg_degrees[desc] > 0){
					--msg_degrees[desc];
				}
				else {
					cerr << "msg type:" << desc->name() << " refer count is 0 !" << endl;
				}
			}
			msg_degrees.erase(it);
		}
	}
};

int main(){
	compiler::DiskSourceTree dst;
	dst.MapPath("", "/usr/local/include");
	dst.MapPath("", ".");
	MFEC mfec;
	compiler::Importer	importer(&dst,&mfec);
	auto ret = importer.Import("test.proto");
	if (!ret){
		std::cerr << "error import" << endl;
		return -1;
	}
	auto pool = importer.pool();
	auto desc = pool->FindMessageTypeByName("Hello");
	GenerateCXXFlat gen(desc);
	gen.print();

	return 0;
}