#pragma once
#include "msg_buffer.hpp"

template<class T>
struct msgproto_t : public T {
	const char * Debug() const {
		return T::ShortDebugString().c_str();
	}
	bool Pack(msg_buffer_t & msgbuf) const {
		int sz = msgbuf.max_size;
		bool ret = Pack(msgbuf.buffer, sz);
		if (!ret) return ret;
		msgbuf.valid_size = sz;
		return true;
	}
	bool Unpack(const msg_buffer_t & msgbuf) {
		return Unpack(msgbuf.buffer, msgbuf.valid_size);
	}
	bool Pack(char * buffer, int & sz) const {
		bool ret = T::SerializeToArray(buffer, sz);
		if (ret) sz = T::ByteSize();
		return ret;
	}
	bool Unpack(const char * buffer, int sz){
		return T::ParseFromArray(buffer, sz);
	}
	int	PackSize() const {
		return T::ByteSize();
	}
};


