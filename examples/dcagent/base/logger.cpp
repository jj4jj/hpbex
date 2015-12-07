#include "logger.h"
#include "stdinc.h"
#include "dcutils.hpp"

struct logger_t {
	logger_config_t	conf;
	//lock info todo ?
	int			last_err;
	string		last_msg;
	FILE	*	pf;
	int			next_rollid;
	bool		inited;
	string		logfile;//current
	logger_t() :last_err(0){
		pf = nullptr;
		next_rollid = 1;
		inited = false;
	}
};

static	logger_t * G_LOGGER = nullptr;
logger_t *		global_logger(){
	return G_LOGGER;
}
int				global_logger_init(const logger_config_t & conf){
	if (G_LOGGER){
		return -1;
	}
	G_LOGGER = logger_create(conf);
	return G_LOGGER ? 0 : -1;
}
void			global_logger_destroy(){
	if (!G_LOGGER){
		return;
	}
	logger_destroy(G_LOGGER);
	G_LOGGER = nullptr;
}
FILE * logfile_open(const char * filename, int & nextrollid){
	FILE * pf = nullptr;
	if (nextrollid == 0){
		pf = fopen(filename, "a");
	}
	else {
		pf = fopen(filename, "w");
	}
	if (pf == nullptr){
		GLOG(LOG_LVL_FATAL, "open file :%s error!", filename);
		return nullptr;
	}
	if (nextrollid == 0){
		char nextfileid[32];
		FILE* rfp = fopen(filename, "r");
		if (rfp){
			size_t sz = sizeof(nextfileid)-1;
			char * pnxf = (char*)nextfileid;
			if (getline(&pnxf, &sz, rfp) > 0){
				nextrollid = strtoul(nextfileid, nullptr, 10);
			}
			fclose(rfp);
		}
		nextrollid = nextrollid > 0 ? nextrollid : 1;
	}
	fprintf(pf, "%d\n", nextrollid);
	return pf;
}
logger_t *	logger_create(const logger_config_t & conf){
	FILE * pf = nullptr;
	int	 nextrollid = 0;
	string filepath = conf.dir + "/" + conf.pattern;
	if (filepath.length() > 1){
		pf = logfile_open(filepath.c_str(), nextrollid);
	}
	logger_t * em = new logger_t();
	if (!em) return nullptr;
	em->last_msg.reserve(conf.max_msg_size);
	em->conf = conf;
	em->inited = true;
	em->next_rollid = nextrollid;
	em->logfile = filepath;
	em->pf = pf;

	return em;
}
void			logger_destroy(logger_t * logger){
	if (logger) {
		if (logger->pf){
			fclose(logger->pf);
		}
		delete logger; 
	}
}
void			logger_set_level(logger_t * logger, log_msg_level_type level){
	if (logger == nullptr){
		logger = G_LOGGER;
	}
	logger->conf.lv = level;
}
int				logger_level(logger_t * logger){
	if (logger == nullptr){
		logger = G_LOGGER;
	}
	if (logger)
		return logger->conf.lv;
	return LOG_LVL_INVALID;
}

//last msg
const char*		logger_msg(logger_t * logger){
	if (logger == nullptr){
		logger = G_LOGGER;
	}
	return logger->last_msg.c_str();
}
//last err
int				logger_errno(logger_t * logger){
	if (logger == nullptr){
		logger = G_LOGGER;
	}
	return logger->last_err;
}

//set last
int				logger_write(logger_t * logger, int loglv, const char* fmt, ...)
{
	if (logger == nullptr){
		logger = G_LOGGER;
	}
	if (loglv < logger->conf.lv){
		return 0;
	}
	//logger lock ?

	va_list ap;
	va_start(ap, fmt);
	int n = 0;
	logger->last_err = errno;
	char * msg_start = (char*)logger->last_msg.data();
	n = vsnprintf(msg_start, logger->last_msg.capacity() - 1, fmt, ap);
	va_end(ap);
	int available_size = logger->last_msg.capacity() - (n + 2);
	char errorno_msg_buff[128];
	if (loglv >= LOG_LVL_WARNING && available_size > 16){
		if (msg_start[n-1] == '\n'){
			--n;
		}
		snprintf(&msg_start[n], available_size, "[system errno:%d(%s)]\n", errno,
			strerror_r(errno, errorno_msg_buff, sizeof(errorno_msg_buff)-1));
	}
	if (logger->pf){
		fputs(msg_start, logger->pf);
		if (ftell(logger->pf) >= logger->conf.max_file_size){
			//shift file <>
			fflush(logger->pf);
			fclose(logger->pf);
			string nextfile = logger->logfile + "." + std::to_string(logger->next_rollid);
			rename(logger->logfile.c_str(), nextfile.c_str());
			int nextrollid = (logger->next_rollid + 1) % logger->conf.max_roll;
			if (nextrollid == 0) nextrollid = 3;
			if ((logger->pf = logfile_open(logger->logfile.c_str(), nextrollid))){
				logger->next_rollid = nextrollid;
			}
		}
	}
	else{
		fputs(logger->last_msg.c_str(), stderr);
	}
	return n;
}
