#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define pr_err(fmt, ...) \
pr_log_level(LOG_ERR, "%s: %d " fmt, __FILE__, __LINE__,\
##__VA_ARGS__);

#define pr_info(fmt, ...) \
pr_log_level(LOG_INFO, "%s: %d " fmt, __FILE__, __LINE__, \
##__VA_ARGS__);

#define pr_warn(fmt, ...) \
pr_log_level(LOG_WARN, "%s: %d " fmt, __FILE__, __LINE__, \
##__VA_ARGS__);

#define BUFSZ 4096

static int log_fd=-1;
static int pos;
static char buf[BUFSZ];

enum {
	LOG_ERR = 0,
	LOG_WARN,
	LOG_INFO,
};

int log_init(char*);
int log_init_fd(int);

int log_init(char* path){
	#ifndef LOG_PATH
	#error Temporary error: LOG_PATH undef
	if (path==NULL)
		return -1;
	#else
	if (path==NULL)
		path=LOG_PATH;
	#endif
	if ((log_fd=open(path, O_CREAT|O_WRONLY, 0666))<0)
		return -1;
}

int log_init_fd(int fd){
	if (fd<0)
		return -1;
	log_fd = fd;
	return 0;
}

/*date pid file-line log-level*/
int pr_log_level(int level, char* fmt, ...){
	char strlevel[6];
	char date[128];
	va_list params;
	char* i;
	time_t t = time(NULL);
	if (log_fd==-1)
		log_init(NULL);
	if (level==0) sprintf(strlevel, "%s ", "err");
	if (level==1) sprintf(strlevel, "%s ", "warn");
	if (level==2) sprintf(strlevel, "%s ", "info");
	strcpy(date, ctime(&t));
	if ((i=strstr(date, "\n"))>=0)
		*i = 0;
	snprintf(buf, BUFSZ, "%s %d %s", date, getpid(), strlevel);
	va_start(params, fmt);
	vsnprintf(buf+strlen(buf), BUFSZ-strlen(buf), fmt, params);
	va_end(params);
	if (level==0&&errno!=0)
		snprintf(buf+strlen(buf), BUFSZ-strlen(buf), ": %s\n", strerror(errno));
	else
		snprintf(buf+strlen(buf), BUFSZ-strlen(buf), "\n");
	if (write(log_fd, buf, strlen(buf))<0)
		return -1;
	return 0;
}
