#include <pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#define NITER 1000000
int count =0;
void *ThreadAdd(void*a)
{
	int i,tmp;
	for(i=0;i<NITER;i++)
	{
		tmp=count;
		tmp=tmp+1;
		count=tmp;

	}
}

int main(int argc,char *argv[])
{
pthread_t tid1,tid2;
if(pthread_create(&tid1,NULL,ThreadAdd,NULL))
{
printf("\nERRORcreatingthread1");
exit(1);
}
if(pthread_create(&tid2,NULL,ThreadAdd,NULL))
{printf("\nERRORcreatingthread2");
exit(1);
}
if(pthread_join(tid1,NULL))
{
printf("\nERRORjoiningthread");
exit(1);
}
if(pthread_join(tid2,NULL))
{
printf("\nERRORjoiningthread");
exit(1);
}
if(count<2*NITER)
printf("\nBOOM!countis[%d],shouldbe %d\n",count,2*NITER);
else
printf("\nOK!countis [%d]\n",count);
pthread_exit(NULL);
}
