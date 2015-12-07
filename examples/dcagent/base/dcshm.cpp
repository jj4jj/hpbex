#include "dcshm.h"


#pragma pack(1)
struct sshm_header_t {
	size_t		size;
};
struct sshm_t {
	sshm_header_t	head;
	char			data[1];
};
#pragma pack()

int _shm_stat(key_t k, size_t size = 0, int ioflag = 0){
	int id = shmget(k, size, IPC_CREAT | IPC_EXCL | ioflag);
	if (id == -1) {
		if (errno == EEXIST) return SHM_EXIST;
		else if (errno == EACCES) return SHM_ERR_PERM;
		else if (errno == EINVAL) return SHM_SIZE_NOT_MATCH;
		else return (SHM_REF_ERRNO + errno);
	}
	else {
		shmctl(id, IPC_RMID, NULL);
		return SHM_OK;
	}
}
int			dcshm_create(const dcshm_config_t & conf, void ** p, bool & attached){
	key_t key = ftok(conf.shm_path.c_str(), 1);
	if (key < 0){
		return SHM_REF_ERRNO + errno;
	}
	int ioflag = 0666;
	size_t realsize = 0;
	if (conf.shm_size > 0){
		realsize = conf.shm_size + sizeof(sshm_header_t); //not aligend
	}
	int err = _shm_stat(key, realsize, ioflag);
    //if conf.attach , shm must be exist
	if (err == SHM_EXIST){
		attached = true;
	}
	else { //not exist . create one
		if (err != SHM_OK){
			//error log err
			return err;
		}
		attached = false;
		if (conf.attach){ //must attach 
			return SHM_NOT_EXIST;
		}
	}
	int flags = IPC_CREAT | ioflag;
	int id = shmget(key, realsize, flags);
	if (id < 0){ //create error
		return SHM_REF_ERRNO + errno;
	}
	void *ap = shmat(id, NULL, ioflag);
	if (ap == (void*)(-1)){
		//attach error
		shmctl(id, IPC_RMID, NULL);
		return SHM_REF_ERRNO + errno;
	}
	sshm_t * shmp = (sshm_t*)(ap);
	if (attached){ //attach checking
		if (realsize > 0 &&
			realsize != shmp->head.size){ //check size
			//error attach
			dcshm_destroy(shmp);
			return SHM_SIZE_NOT_MATCH;
		}
	}
	else {
		shmp->head.size = realsize;
	}
	*p = &(shmp->data[0]);
	return SHM_OK;
}
void	 dcshm_destroy(void * p){
	if (p){
		sshm_t * shmp = (sshm_t*)((char*)p - sizeof(sshm_header_t));
		shmdt(shmp);
	}
}
