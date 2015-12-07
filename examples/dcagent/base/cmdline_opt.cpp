#include <getopt.h>
extern char *optarg;
extern int optind, opterr, optopt;
#include "dcutils.hpp"
#include "cmdline_opt.h"

struct cmdline_opt_impl_t {
	int								argc;
	char **							argv;
	std::multimap<string, string>	dict_opts;
	std::map<string, string>		dict_opts_default;
	string							usage;
};

#define _THIS_HANDLE ((cmdline_opt_impl_t*)handle)

cmdline_opt_t::cmdline_opt_t(int argc, char ** argv){
	handle = new cmdline_opt_impl_t();
	_THIS_HANDLE->argc = argc;
	_THIS_HANDLE->argv = argv;
	_THIS_HANDLE->usage = "usage: ";
	_THIS_HANDLE->usage += argv[0];
	_THIS_HANDLE->usage += " [options]	\n";
	_THIS_HANDLE->usage += "option should be like as follow:\n";
}
cmdline_opt_t::~cmdline_opt_t(){
	if (handle){
		delete _THIS_HANDLE;
	}
}
// = "version:n:v:desc:default;log-path:r::desc;:o:I:desc:default"
void
cmdline_opt_t::parse(const char * pattern){
	std::vector<std::string>	sopts;
    string pattern_ex = "help:n:h:show the help info:;";
    pattern_ex += pattern;
	dcsutil::split(pattern_ex.c_str(), ";", sopts);
	std::vector<struct option>	longopts;
	std::vector<std::string>	longoptnames;
	longoptnames.reserve(128);
	string short_opt;
	for (auto & sopt : sopts){
		std::vector<std::string>	soptv;
		dcsutil::split(sopt, ":", soptv, false, 5);
		if (soptv.size() < 4){
			std::cerr << "error format option:" << sopt << " size:" << soptv.size() << " but < 4" << std::endl;
			std::cerr << "pattern opt must be format of '[<long name>]:[rno]:[<short name>]:[desc]:[default];' " << std::endl;
			exit(-2);
		}
		if (soptv[2][0]){
			short_opt += soptv[2][0];
			if (soptv[1][0] == 'r'){
				short_opt += ":";
			}
			else if (soptv[1][0] == 'o'){
				short_opt += "::";
			}
		}
		if (soptv[0][0]){
			longoptnames.push_back(soptv[0]);
			struct option opt_;
			opt_.name = longoptnames.back().c_str();
			opt_.flag = NULL;
			opt_.has_arg = no_argument;
			opt_.val = 0;
			if (soptv[1][0] == 'o'){
				opt_.has_arg = optional_argument;
			}
			else if (soptv[1][0] == 'r'){
				opt_.has_arg = required_argument;
			}
			if (soptv[2][0]){
				opt_.val = soptv[2][0];
			}
			longopts.push_back(opt_);
			//std::cout << "add dbg:" << opt_.name << ":val:" << opt_.val << std::endl;
		}
		//////////////////////////////////////////////////////////
		dcsutil::strrepeat(_THIS_HANDLE->usage, " ", 4);
		int length = 4;
		if (soptv[2][0]){
			_THIS_HANDLE->usage += "-";
			_THIS_HANDLE->usage += soptv[2];
			length += 2;
		}
		if (soptv[0][0]){
			if (soptv[2][0]){
				_THIS_HANDLE->usage += ", ";
				length += 2;
			}
			_THIS_HANDLE->usage += "--";
			_THIS_HANDLE->usage += soptv[0];
			length += soptv[0].length();
			length += 2;
		}
		if (soptv[1][0] == 'r'){
			_THIS_HANDLE->usage += " <arg>";
			length += 6;
		}
		if (soptv[1][0] == 'o'){
			_THIS_HANDLE->usage += " [arg]";
			length += 6;
		}
        #define MAX_OPT_SHOW_TEXT_WIDTH (25)
		if (soptv[3][0]){
			if (length < MAX_OPT_SHOW_TEXT_WIDTH){
				dcsutil::strrepeat(_THIS_HANDLE->usage, " ", MAX_OPT_SHOW_TEXT_WIDTH - length);
			}
			_THIS_HANDLE->usage += "\t";
			_THIS_HANDLE->usage += soptv[3];
		}
		if (soptv.size() > 4 && soptv[4][0]){
			_THIS_HANDLE->usage += "  (";
			_THIS_HANDLE->usage += soptv[4];
			_THIS_HANDLE->usage += ")";
			/////////////////////////////////////
			_THIS_HANDLE->dict_opts_default[string(soptv[0])] = soptv[4];
			_THIS_HANDLE->dict_opts_default[string(soptv[2])] = soptv[4];
		}
		_THIS_HANDLE->usage += "\n";
		//////////////////////////////////////////////////////////
	}
	//end
	struct option end_opt_ = { NULL, no_argument, NULL, 0 };
	longopts.push_back(end_opt_);
	///////////////////////////////////////////////////////////////////////////
	int longIndex = 0;
	int opt = 0;
	//std::cout << "dbg:" << short_opt << std::endl;
	opt = getopt_long(_THIS_HANDLE->argc, _THIS_HANDLE->argv, short_opt.c_str(), &longopts[0], &longIndex);
	while (opt != -1) {
		if (opt == 0){
			string opt_name = longopts[longIndex].name;
			string opt_value = (optarg ? optarg : "");
			//std::cout << "dbg long:" << opt_name << "=" << opt_value << ":length:" << opt_value.length() << std::endl;
			_THIS_HANDLE->dict_opts.insert(std::make_pair(opt_name, opt_value));
			if (longopts[longIndex].val > 0){
				opt_name = string((char*)&(longopts[longIndex].val), 1);
				//std::cout << "dbg short:" << opt_name << "=" << opt_value << ":length:" << opt_value.length() << std::endl;
				_THIS_HANDLE->dict_opts.insert(std::make_pair(opt_name, opt_value));
			}
            //std::clog << "opt_name" << "dbg"<< std::endl;
			if (opt_name == "help"){
				pusage();
			}
		}
		else if (opt == '?'){
			//usage
			pusage();
		}
		else {	//short opt
            if(opt == 'h'){
                pusage();
            }
			string opt_name = string((char*)&opt, 1);
			string opt_value = (optarg ? optarg : "");
			_THIS_HANDLE->dict_opts.insert(std::make_pair(opt_name, opt_value));
			//std::cout << "dbg short:" << opt_name << "=" << opt_value << ":length:" << opt_value.length() << std::endl;
		}
		opt = getopt_long(_THIS_HANDLE->argc, _THIS_HANDLE->argv, short_opt.c_str(), &longopts[0], &longIndex);
	}
}
int			
cmdline_opt_t::getoptnum(const char * opt){
	auto range = _THIS_HANDLE->dict_opts.equal_range(opt);
	int count = 0;
	while (range.first != range.second){
		++count;
		range.first++;
	}
	return count;
}
const char * 
cmdline_opt_t::getoptstr(const char * opt, int idx){
	auto range = _THIS_HANDLE->dict_opts.equal_range(opt);
	while (range.first != range.second){
		if (idx == 0){
			return range.first->second.c_str();
		}
		--idx;
		range.first++;
	}
	auto it = _THIS_HANDLE->dict_opts_default.find(opt);
	if (it != _THIS_HANDLE->dict_opts_default.end()){
		return it->second.c_str();
	}
	return nullptr;
}
int			 
cmdline_opt_t::getoptint(const char * opt, int idx){
	const char * value = getoptstr(opt, idx);
	if (value){
		return atoi(value);
	}
	return 0;
}
void
cmdline_opt_t::pusage(){
	std::cerr << _THIS_HANDLE->usage << std::endl;
    exit(-1);
}
const char *	
cmdline_opt_t::usage(){
	return _THIS_HANDLE->usage.c_str();
}

