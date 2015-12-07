#pragma once

struct msg_buffer_t {
	char *	buffer;
	int		valid_size;
	int		max_size;
	msg_buffer_t(const char * csp = nullptr, int sz = 0) :buffer((char*)csp), valid_size(sz), max_size(0){
		if (csp != nullptr && sz == 0){
			valid_size = strlen(csp) + 1;
		}
	}
	msg_buffer_t(const string & str) :msg_buffer_t(str.c_str(), str.length() + 1){}
	int create(int max_sz){
		destroy();
		char * p = (char*)malloc(max_sz);
		if (!p) return -1;
		bzero(p, max_sz);
		buffer = p;
		max_size = max_sz;
		valid_size = 0;
		return 0;
	}
	void destroy()
	{
		if (buffer && max_size > 0) {
			free(buffer); buffer = nullptr;
			valid_size = max_size = 0;
		}
	}
};
