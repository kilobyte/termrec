#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"

int bps,wait;
int n,d,dtime;
char buf[1024],*b;
struct timeval tv,lv;

int main(int argc,char **argv)
{
	if ((argc!=2)||((bps=atoi(argv[1]))<=0)||!(wait=8000000/bps))
	{
		write(2,"Specify a baudrate!\n",20);
		return 1;
	};
	gettimeofday(&lv,0);
	dtime=0;
	
	while ((n=read(0,buf,1024)))
	{
		if (n==-1)
			return 1;
		b=buf;
		while (n)
		{
			usleep(wait);
			gettimeofday(&tv,0);
			dtime+=(tv.tv_sec-lv.tv_sec)*1000000+(tv.tv_usec-lv.tv_usec);
			lv.tv_sec=tv.tv_sec;
			lv.tv_usec=tv.tv_usec;
			d=dtime/wait;
			if (!d)
				d=1;
			else
				if (d>n)
					d=n;
			write(1,b,d);
			b+=d;
			dtime-=d*wait;
			n-=d;
		}
	};
	
	return 0;
}
