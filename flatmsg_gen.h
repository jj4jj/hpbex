#pragma once

#include "ext_meta.h"
#include "google/protobuf/compiler/importer.h"
#include <iostream>
#include <fstream>

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;

extern std::stringstream error_stream;

/*
struct TemplateMsg_GenerateCXXFlat {
	int32_t		x;

	void	construct();
	void	convto();
	void	convfrom();
	//bool	operator == () const ;
	//bool	operator < () const ;
	int		find_idx_x();
	int		append_x();
	int		insert_x();
	int		remove_x();
	////////////////////////
	int		lower_bound_x();
	int		upper_bound_x();
	int		binsert_x();
};
*/


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
	static string repeat(const char * s, int n){
		string str = "";
		for (int i = 0; i < n; ++i){
			str += s;
		}
		return str;
	}
	void	DumpFile(const char * file){
		ofstream ofs(file, ios::out);
		ofs << GetCodeText() << endl;
	}
	string	GetCodeText(){
		TopologySort(ordered_desc);
		std::stringstream oss;
		oss << "#pragma once" << endl << endl;
		oss << "//this file is auto generated by (hbpex)[https://github.com/jj4jj/hpbex.git] , don't edit this file !" << endl;
		time_t t_m = time(NULL);
		oss << "//file generate datetime: " << asctime(localtime(&t_m)) << endl;

		const string proto_name_post_fix = ".proto";
		string this_proto_heder_file_name = root->file()->name();
		this_proto_heder_file_name.replace(this_proto_heder_file_name.find(proto_name_post_fix), proto_name_post_fix.length(), ".pb.h");
		//import default
		oss << "#include <algorithm>" << endl;
		//import
		for (int i = 0; i < root->file()->dependency_count(); ++i){
			auto f = root->file()->dependency(i);
			string ns_name = f->name();
			if (ns_name == "extensions.proto"){
				continue;
			}
			ns_name.replace(ns_name.find(proto_name_post_fix), proto_name_post_fix.length(), ".hpb.h");
			oss << "#include \"" << ns_name << "\"" << endl;
		}
		//self proto .pb.h
		oss << "#include \"" << this_proto_heder_file_name << "\"" << endl;
		oss << endl;
		//package begin
		if (!root->file()->package().empty()){
			oss << "namespace " << root->file()->package() << " { " << endl;
		}
		oss << endl;
		//declaration using
		for (int i = 0; i < (int)ordered_desc.size(); ++i){
			string ns_name = ordered_desc[i]->file()->package();
			if (!ns_name.empty() && ns_name != root->file()->package()){
				//oss << "using " << ns_name << "::" << ordered_desc[i]->name() << endl;
				oss << "using " << ns_name << "::" << EXTMetaUtil::GetStructName(ordered_desc[i]) << ";" << endl;
			}
		}
		oss << endl;

		//convert
		for (int i = 0; i < (int)ordered_desc.size(); ++i){
			oss << ConvertMsg(ordered_desc[i]);
		}
		oss << endl;

		//package end
		if (!root->file()->package().empty()){
			oss << "} // end of namespace: " << root->file()->package() << endl;
		}
		return oss.str();
	}
	string	ConvertMsg(const Descriptor * desc, const char * indent = "    "){
		stringstream ss_convert_msg;
		int level = 0;
		char sz_line_buffer[1024];
#define	WRITE_LINE(format, ...)		do{\
	snprintf(sz_line_buffer, sizeof(sz_line_buffer), format, ##__VA_ARGS__); \
	ss_convert_msg << GenerateCXXFlat::repeat(indent, level) << sz_line_buffer << std::endl; \
		}while (false)
#define	WRITE_STR(format, ...)		do{\
	snprintf(sz_line_buffer, sizeof(sz_line_buffer), format, ##__VA_ARGS__); \
	ss_convert_msg << GenerateCXXFlat::repeat(indent, level) << sz_line_buffer; \
		}while (false)
		////////////////////////////////////////////////////////////////////////////////////
		if (desc->file()->package() != root->file()->package()){
			return "";
			//WRITE_LINE("using %s::%s;", desc->file()->package().c_str(), EXTMetaUtil::GetStructName(desc).c_str());
			//return ss_convert_msg.str();
		}

		//struct Msg {
		WRITE_LINE("struct %s {", EXTMetaUtil::GetStructName(desc).c_str());
		level++;
		std::vector<EXTFieldMeta>	fieldmeta_list;

		EXTMessageMeta	msg_meta;
		if (msg_meta.AttachDesc(desc)){
			throw logic_error("parse error !");
		}

		for (auto & sfm : msg_meta.sub_fields){
			if (sfm.f_count.empty()){
				WRITE_LINE("%s\t\t%s;", sfm.GetTypeName().c_str(), sfm.GetVarName().c_str());
			}
			else {
				WRITE_LINE("struct {size_t count; %s list[%s];}\t\t%s;",
					sfm.GetTypeName().c_str(), sfm.f_count.c_str(), sfm.GetVarName().c_str());
			}
			fieldmeta_list.push_back(sfm);
		}
		///////////////////////////////////////////////////////////////////////////
		WRITE_LINE("\t");
		WRITE_LINE("//////////////member functions///////////////////////////////////");
		//construct
		WRITE_LINE("void\t\tconstruct() {");
		++level;
		WRITE_LINE("if(sizeof(*this) < 1024) { memset(this,0,sizeof(*this)); return ;}");
		for (auto & sfm : msg_meta.sub_fields){
			//set
			string st_var_name = sfm.GetVarName();
			if (!sfm.f_count.empty()){
				WRITE_LINE("%s.count = 0U;", st_var_name.c_str());
			}
			else {
				if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
					WRITE_LINE("%s.data[0] = 0;", st_var_name.c_str());
				}
				else if (sfm.field_desc->type() == FieldDescriptor::TYPE_BYTES){
					WRITE_LINE("%s.length = 0U;", st_var_name.c_str());
				}
				else if (sfm.field_desc->type() == FieldDescriptor::TYPE_ENUM){
					WRITE_LINE("%s = %s;//should be 0", st_var_name.c_str(),
						sfm.field_desc->default_value_enum()->name().c_str());
					if (sfm.field_desc->default_value_enum()->number() != 0){
						WRITE_LINE("#warning \"enum type:%s default value should be 0 value define but %s != 0 \"",
							sfm.field_desc->default_value_enum()->type()->name().c_str(),
							sfm.field_desc->default_value_enum()->name().c_str());
						WRITE_LINE("assert( \"enum default value should be 0 value define !\" && %s == 0 );",
							sfm.field_desc->default_value_enum()->name().c_str());
					}
				}
				else if (sfm.field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
					WRITE_LINE("%s.construct();", st_var_name.c_str());
				}
				else {// (sfm.field_desc->cpp_type() <= FieldDescriptor::CPPTYPE_ENUM)
					WRITE_LINE("%s = 0;", st_var_name.c_str());
				}
			}
		}
		--level;
		WRITE_LINE("}");
		//if memset()

		///////////////////////////////////////////////////////////////////////////
		//convto
		WRITE_LINE("void\t\tconvto(%s & convtomsg_) const {", desc->name().c_str());
		++level;
		for (auto & sfm : msg_meta.sub_fields){
			//set
			string st_var_name = sfm.GetVarName();
			if (!sfm.f_count.empty()){
				WRITE_LINE("assert(%s.count <= %s);//assertion", st_var_name.c_str(), sfm.f_count.c_str());
				//for(size_t i = 0;i < a.count; ++i)
				WRITE_LINE("for ( size_t i = 0; i < %s.count && i < (sizeof(%s.list)/sizeof(%s.list[0])); ++i){",
					st_var_name.c_str(), st_var_name.c_str(), st_var_name.c_str());
				st_var_name += ".list[i]";
				++level;
			}
			WRITE_LINE("%s;", sfm.GetScalarConvToMeth("convtomsg_", st_var_name, sfm.field_desc->name()).c_str());
			if (!sfm.f_count.empty()){
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
			if (!sfm.f_count.empty()){
				WRITE_LINE("assert(convfrommsg_.%s_size() <= %s);//assertion", msg_var_name.c_str(), sfm.f_count.c_str());
				//for(size_t i = 0;i < a.count; ++i)
				WRITE_LINE("%s.count = 0;", sfm.GetVarName().c_str());
				WRITE_LINE("for ( size_t i = 0; i < (size_t)convfrommsg_.%s_size() && i < (sizeof(%s.list)/sizeof(%s.list[0])); ++i){", msg_var_name.c_str(), msg_var_name.c_str(), msg_var_name.c_str());
				st_var_name += ".list[i]";
				msg_var_name += "(i)";
				++level;
			}
			else {
				msg_var_name += "()";
			}
			WRITE_LINE("%s;", sfm.GetScalarConvFromMeth("convfrommsg_", st_var_name, msg_var_name.c_str()).c_str());
			if (!sfm.f_count.empty()){
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
			}
		}
		--level;
		WRITE_LINE("}");
		///////////////////////////////////////////////////////////////////////////////
		//int compare
		WRITE_LINE("int\t\tcompare(const %s & rhs_) const {", EXTMetaUtil::GetStructName(desc).c_str());
		++level;
		WRITE_LINE("int _cmp_ret = 0;");
		//
		string var_name;
		for (size_t i = 0; i < msg_meta.pks_fields.size(); ++i){
			auto & sfm = *msg_meta.pks_fields[i];
			var_name = sfm.GetVarName();
			if (!sfm.f_count.empty()){
				WRITE_LINE("for (size_t i = 0; i < %s.count && i < rhs_.%s.count; ++i) {",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				var_name += ".list[i]";
				++level;
			}
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_FLOAT ||
				sfm.field_desc->type() == FieldDescriptor::TYPE_DOUBLE){
				WRITE_LINE("#warning \"%s\";", "comparing variable is a float number , maybe not precise .");
				WRITE_LINE("_cmp_ret = ((%s + 10e-6) < rhs_.%s) ? -1 : (%s > (rhs_.%s + 10e-6) ? 1 : 0);",
				var_name.c_str(), var_name.c_str(),
				var_name.c_str(), var_name.c_str());
			}
			else if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("_cmp_ret = strncmp(%s.data, rhs_.%s.data, %s);",
					var_name.c_str(), var_name.c_str(),
					sfm.f_length.c_str());
			}
			else if (sfm.field_desc->type() == FieldDescriptor::TYPE_BYTES){
				WRITE_LINE("_cmp_ret = memcmp(%s.data, rhs_.%s.data, std::min(%s.length, rhs_%s.length));",
					var_name.c_str(), var_name.c_str(),
					var_name.c_str(), var_name.c_str());
				WRITE_LINE("if (_cmp_ret == 0) { _cmp_ret = (%s.length < rhs_%s.length) ? -1 : ((%s.length == rhs_%s.length): 0: 1); }",
					var_name.c_str(), var_name.c_str(),
					var_name.c_str(), var_name.c_str());
			}
			else if (sfm.field_desc->type() == FieldDescriptor::TYPE_MESSAGE){
				WRITE_LINE("_cmp_ret = %s.compare(rhs_.%s);",
					var_name.c_str(), var_name.c_str());
			}
			else {
				WRITE_LINE("_cmp_ret = (%s < rhs_.%s) ? -1 : ((%s == rhs_.%s) ? 0 : 1);",
					var_name.c_str(), var_name.c_str(),
					var_name.c_str(), var_name.c_str());
			}
			WRITE_LINE("if (_cmp_ret != 0) { return _cmp_ret; }");
			if (!sfm.f_count.empty()){
				//end for(size_t i = 0 ;i < count ; ++i){
				--level;
				WRITE_LINE("}");
				WRITE_LINE("_cmp_ret = (%s.count < rhs_.%s.count ) ? -1 : ((%s.count == rhs_.%s.count ) ? 0 : 1);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(),
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
			}
		}
		WRITE_LINE("return 0;");
		--level;
		WRITE_LINE("}");

		//bool operator ==  //for find
		WRITE_LINE("bool\t\toperator == (const %s & rhs_) const {", EXTMetaUtil::GetStructName(desc).c_str());
		++level;
		WRITE_LINE("return this->compare(rhs_) == 0;");
		--level;
		WRITE_LINE("}");

		//bool operator <  //for less //////////////////////////////////////////
		WRITE_LINE("bool\t\toperator < (const %s & rhs_) const {", EXTMetaUtil::GetStructName(desc).c_str());
		++level;
		WRITE_LINE("return this->compare(rhs_) < 0;");
		--level;
		WRITE_LINE("}");

		//////////////////////////////list support////////////////////////////////////////////////////
		for (auto & sfm : msg_meta.sub_fields){
			if (sfm.f_count.empty()){
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
				WRITE_LINE("auto it = std::find_if(%s.list, %s.list + %s.count, ",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				++level;
				WRITE_LINE("[&entry_](decltype(%s.list[0]) & st) ->bool { return entry_ == st.data; }); ",
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
				WRITE_LINE("if ( entry_.length() >= %s ) { return -1; }", sfm.f_length.c_str());
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.f_count.c_str());
				++level;
				//%s.list[count] = entry_;
				//++count;
				WRITE_LINE("strncpy(%s.list[%s.count].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.f_length.c_str());
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
					sfm.f_count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("strncpy(%s.list[%s - 1].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.f_count.c_str(), sfm.f_length.c_str());
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
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.f_count.c_str());
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
					sfm.f_count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("%s.list[%s - 1] = entry_;",
					sfm.GetVarName().c_str(), sfm.f_count.c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("return 0;");
				--level;
				WRITE_LINE("}");
			}
			//-------insert_ ------------------------------------------------
			//int	insert_x(int idx_, entry & v , bool shift_force_insert = false)
			if (sfm.field_desc->type() == FieldDescriptor::TYPE_STRING){
				WRITE_LINE("int\t\tinsert_%s(size_t idx_, const std::string & entry_, bool shift_force_insert_ = true) {",
					sfm.GetVarName().c_str());
				++level;
				//if entry_.length() >= count return -1
				WRITE_LINE("if ( entry_.length() >= %s ) { return -1; }", sfm.f_length.c_str());
				WRITE_LINE("if ( idx_ > %s.count ) { return -2; }", sfm.GetVarName().c_str());
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.f_count.c_str());
				++level;
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s.count - idx_)*sizeof(%s.list[0]));",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("strncpy(%s.list[idx_].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.f_length.c_str());
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("else {");
				++level;
				WRITE_LINE("if ( !shift_force_insert_ ) { return -3; }");
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s - 1 - idx_)*sizeof(%s.list[0]));",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.f_count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("strncpy(%s.list[idx_].data, entry_.c_str(), %s-1);",
					sfm.GetVarName().c_str(), sfm.f_length.c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("return 0;");
				--level;
				WRITE_LINE("}");
			}
			else {
				WRITE_LINE("int\t\tinsert_%s(size_t idx_, const %s & entry_, bool shift_force_insert_ = false) {",
					sfm.GetVarName().c_str(), sfm.GetScalarTypeName().c_str());
				++level;
				WRITE_LINE("if ( idx_ >= %s || idx_ > %s.count ) { return -2; }",
					sfm.f_count.c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("if ( %s.count < %s ) {", sfm.GetVarName().c_str(), sfm.f_count.c_str());
				++level;
				//->
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s.count - idx_)*sizeof(%s.list[0]));",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.GetVarName().c_str());
				WRITE_LINE("%s.list[idx_] = entry_;", sfm.GetVarName().c_str());
				WRITE_LINE("++%s.count;", sfm.GetVarName().c_str());
				--level;
				WRITE_LINE("}");
				WRITE_LINE("else {");
				++level;
				WRITE_LINE("if ( !shift_force_insert_ ) { return -3; }");
				WRITE_LINE("memmove(%s.list + idx_ + 1, %s.list + idx_, (%s - 1 - idx_)*sizeof(%s.list[0]));",
					sfm.GetVarName().c_str(), sfm.GetVarName().c_str(), sfm.f_count.c_str(), sfm.GetVarName().c_str());
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
				WRITE_LINE("[](decltype(%s.list[0]) & t2, const std::string & t1) ->bool { return t1 < t2.data; }); ",
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
				WRITE_LINE("[](const std::string & t1,decltype(%s.list[0]) & t2) ->bool { return t1 < t2.data; }); ",
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
		return ss_convert_msg.str();
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
					if (md != (int)reverse_refer[field].size()){
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
