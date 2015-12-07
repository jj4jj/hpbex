#include "base/logger.h"
#include "google/protobuf/message.h"
#include "util_proto.h"

NS_BEGIN(dcsutil)

static void 
_protobuf_global_logger_printer(google::protobuf::LogLevel level, 
	const char* filename, int line,
	const std::string& message){
	log_msg_level_type	loglv;
	switch (level){
	case google::protobuf::LOGLEVEL_INFO:
		loglv = LOG_LVL_INFO;
	case google::protobuf::LOGLEVEL_WARNING:
		loglv = LOG_LVL_WARNING;
	case google::protobuf::LOGLEVEL_ERROR:
		loglv = LOG_LVL_WARNING;
	case google::protobuf::LOGLEVEL_FATAL:
		loglv = LOG_LVL_FATAL;
	default:
		loglv = LOG_LVL_DEBUG;
	}
	GLOG(loglv, "protobuf log info (lv:%d filename:%s:%d msg:%s)",
		level, filename, line, message.c_str());
}
void			protobuf_logger_init(logger_t * logger){
	google::protobuf::SetLogHandler(_protobuf_global_logger_printer);
}


NS_END()