#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

int main(){
	uint64_t volume=370440;
	int fin, sfd, fout;
	char buf[65536];
	uint64_t len;
	fin=open("./in", O_WRONLY);
	fout=open("./out", O_RDONLY);
//	read(fout, &volume, 8);
	sfd=open("/home/qwerty/Изображения/1.jpg", O_RDONLY);
	write(fin, &volume, 8);
	for (volume; (int64_t)(volume-65536)>0; volume-=65536){
	        read(sfd, buf, 65536);
                write(fin, buf, 65536);
        }
        read(sfd, buf, volume);
        write(fin, buf, volume);
	do{	if (read(fout, &len, 8)<=0)
			break;
		if (len==0)
			break;
		if (read(fout, buf, len)<=0)
			break;
		//write(sfd, buf, len);
	} while (len!=0)
	close(fin);
	close(sfd);
	close(fout);
	return 0;
}
