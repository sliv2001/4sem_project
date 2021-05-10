#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <err.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#define LOG_PATH "./log.txt"
#include "logger.h"

#define PEER_MAX 512
#define SCRIPT_PATH "./find.py"
#define IPC_PATH "./semaphore"
#define IN_FIFO "./in"
#define OUT_FIFO "./out"

static int Port=-1;
static int sfd = -1;
static int script_on = -1;
static int sem=-1;
static int csfd = -1;
static struct sockaddr_in peer;
static int fin=-1, fout=-1;

void sig(int signo){
	pid_t pid = wait(NULL);
	pr_info("Finalized process-child with pid=%d", pid);
	if (signal(SIGCHLD, &sig)==SIG_ERR)
		pr_warn("Multiple zombies possible");
}
int contains(int argc, char** argv, const char* val){
	int i;
	for (i=1; i<argc; i++){
	        if (!strcasecmp(val, argv[i]))
		return i;
	}
	return 0;
}

int parse(int argc, char** argv){
	int i, result=0;
	if ((i=contains(argc, argv, "-p"))>=0){
		if (argc>i+1)
			sscanf(argv[i+1], "%d", &Port);
		else
			result=-1;
	}
	else
		result = -1;
	return result;
}

int finalize(){
	int res = semctl(sem, 0, IPC_RMID);
	if (res<0)
		pr_warn("Couldnot release semaphore set");
	close(sfd);
}

int finalizeChild(){
	close(csfd);
	close(fin);
	close(fout);
	return 0;
}

int setup_socket(){
	int res;
        struct sockaddr_in addr;
        struct in_addr internet_adress;
	if ((sfd=socket(AF_INET, SOCK_STREAM, 0))<0){
		pr_err("Couldnot create socket");
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
        memset(&internet_adress, 0, sizeof(internet_adress));
        addr.sin_family = AF_INET;
        internet_adress.s_addr = htonl(INADDR_ANY);
        addr.sin_addr = internet_adress;
        addr.sin_port = htons(Port);
	if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr))<0){
		pr_err("Couldnot bind socket");
		finalize();
		return -1;
	}
	if (listen(sfd, PEER_MAX)<0){
		pr_err("Couldnot setup socket for listening");
		finalize();
		return -1;
	}
	return 0;
}

int setup_script(){
	int res = mkfifo(OUT_FIFO, 0666);
	pid_t pid;
	res-=mkfifo(IN_FIFO, 0666);
	if (res<0&&errno!=EEXIST){
		pr_err("Couldnot create fifo for script");
		return -1;
	}
	if ((pid=fork())<0){
		pr_err("Failure at forking to script");
		return -1;
	}
	if (pid==0)
		execlp("python3", "python3", SCRIPT_PATH, (char*)0);
	script_on=0;
	return 0;
}

int setSem(int read){
	struct sembuf semb;
	int done=0;
	semb.sem_num = read;
	semb.sem_op = -1;
	semb.sem_flg = SEM_UNDO;
	return semop(sem, &semb, 1);
}

int unsetSem(int read){
	struct sembuf semb;
	semb.sem_num = read;
	semb.sem_op = 1;
	semb.sem_flg = SEM_UNDO;
	return semop(sem, &semb, 1);
}

int initSem(){
	int res = 0;
	union semun {
               int              val;
               struct semid_ds *buf;
               unsigned short  *array;
               struct seminfo  *__buf;
        } s;
	s.val = 1;
	res -= semctl(sem, 0, SETVAL, s);
	res -= semctl(sem, 1, SETVAL, s);
	return res;
}

int setup_sem(){
	key_t key = ftok(IPC_PATH, 0);
	sem = semget(key, 2, IPC_CREAT|IPC_EXCL|0666);
	if (key<0){
		pr_err("Couldnot comlete ftok()");
		return -1;
	}
	if (sem<0){
		sem = semget(key, 2, IPC_CREAT|0666);
		if (sem<0){
			pr_err("Couldnot get semaphore ID");
			return -1;
		}
		pr_warn("Semaphore set already exists");
	}
	if (initSem()<0){
		pr_err("Couldnot set semaphore value");
		return -1;
	}
	return 0;
}

int getData(char* buf, int sz){
	int r;
	if ((r=read(csfd, buf, sz))<0){
		finalizeChild();
		return -1;
	}
	if (r<sz)
	if (getData(buf+r, sz-r)<=0)
		return 0;
	return sz;
}

int childProcess(){
	uint64_t volume;
	char buf[65536];
	int r;
	if ((r=recv(csfd, &volume, 8, 0))<=0)
		if (r<=0){
			pr_info("socket disconnected");
			exit(0);
		}
		if (r<8)
			return 0;

	if (script_on==0){
		setSem(0);
		fin=open(IN_FIFO, O_WRONLY);
		fout=open(OUT_FIFO, O_RDONLY);
		if (fin<0||fout<0){
			pr_err("Couldnot open fifo");
			finalizeChild();
			exit(0);
		}
		write(fin, &volume, 8);
		for (volume; (int64_t)volume-65536>0; volume-=65536){
			getData(buf, 65536);
			write(fin, buf, 65536);
		}
		if (read(csfd, buf, volume)<=0){
			finalizeChild();
			exit(0);
		}
		write(fin, buf, volume);
		do{     if (read(fout, &volume, 8)<=0)
        	                break;
			write(csfd, &volume, 8);
                	if (volume==0)
                        	break;
	                if (read(fout, buf, volume)<=0)
        	                break;
			write(csfd, buf, volume);
                } while (volume!=0);
	        close(fin);
	        close(fout);
		unsetSem(0);
	}
	return 0;
}

int main(int argc, char** argv){
	int size = sizeof(struct sockaddr_in);
	pid_t pid;
	if (log_init_fd(STDERR_FILENO)<0){
		printf("Couldnot initialize logger");
		return -1;
	}
	if (signal(SIGCHLD, &sig)==SIG_ERR)
		pr_warn("Multiple zombies possible");
	if (parse(argc, argv)<0){
		pr_err("Wrong parsing results");
		return -1;
	}
	if (setup_socket()<0){
		pr_err("Couldnot initialize socket");
		return -1;
	}
	if ((script_on = setup_script())<0)
		pr_warn("Couldnot initialize script %s", SCRIPT_PATH);
	if (setup_sem()<0){
		pr_err("Couldnot initialize semaphores");
		finalize();
		return -1;
	}
	while (1){
		csfd = accept(sfd, (struct sockaddr*)&peer, (socklen_t*)&size);
		if (csfd<0){
			pr_warn("Wrong acception result");
			continue;
		}
		if ((pid = fork())<0){
			pr_warn("Failure in forking");
			close(csfd);
		}
		if (pid>0)
			close(csfd);
		if (pid==0)
			break;
	}
	while (1)
		childProcess();
	finalize();
	return 0;
}
