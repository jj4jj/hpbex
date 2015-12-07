#pragma once

#include "logger.h"
#include "dcutils.hpp"

struct		profile_t {
	const char *  file;
	const char *  funcname;
	const int  line;
	static std::string	 level_s;
	uint64_t			 start_time;
	profile_t(const char * file_, const char * funcname_, int line_) :
		file(file_), funcname(funcname_), line(line_){
			start_time = dcsutil::time_unixtime_us();
			if (!is_on(start_time)){
				return;
			}
			LOGR(LOG_LVL_PROF, "%s%s:%d BEGIN", level_s.c_str(), funcname, line);
			level_s.push_back('\t');
		}
	~profile_t(){
		uint64_t now_us = dcsutil::time_unixtime_us();
		if (!is_on(now_us)){
			return;
		}
		uint64_t lcost_time = now_us - start_time;
		level_s.pop_back();
		LOGR(LOG_LVL_PROF, "%s%s:%d END | COST:%lu us", level_s.c_str(), funcname, line, lcost_time);
	}
private:
	static uint64_t s_prof_time_end;
	static inline bool is_on(uint64_t current){
		return current < s_prof_time_end;
	}
public:
	static void on(int time_s){
		s_prof_time_end = dcsutil::time_unixtime_us() + time_s * 1000000;
	}

};

//#define _LOG_PROFILING_

#define PROFILE_ON(s)	profile_t::on((s))
#ifdef _LOG_PROFILING_
#define PROFILE_FUNC() profile_t _func_prof_(__FILE__,__FUNCTION__,__LINE__)
#else
#define PROFILE_FUNC() 
#endif