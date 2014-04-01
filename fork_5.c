#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

int main()
{
	int i;
	pid_t pid = fork();
	for(i=0; i<5 ; i++)
	{
		if(pid)
		{
			printf("*****In Parent Process. Parent PID is %d. "
					      "Child PID is %d*****\n", getppid(), pid);
			pid = fork();
		}
		else
			printf("In Child Process. Child PID is %d\n", getpid());
		fflush(stdout);
	}
	exit(0);
}	
