#pragma once
#include "stdinc.h"

struct dcshm_config_t {
	string	shm_path;
	int		shm_size;
	bool	attach; //just attach , no create
};
struct sshm_t;
enum dcshm_error_type {
	SHM_OK = 0,
	SHM_EXIST = 1,
	SHM_NOT_EXIST = 2,
	SHM_SIZE_NOT_MATCH = 3,
	SHM_ERR_PERM = 4,
	SHM_REF_ERRNO = 0x7FFF, //+errno
};

int			dcshm_create(const dcshm_config_t & conf, void ** p, bool & attached);
void		dcshm_destroy(void *);







