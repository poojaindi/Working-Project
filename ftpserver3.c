/* Server program of a file transfer service. 
   This is a "concurrent server" that can
   handle requests from multiple simultaneous clients.
   For each client:
    - get file name and check if it exists
    - send size of file to client
    - send file to client, a block at a time
    - close connection with client
*/

#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <sys/signal.h>
#include <pthread.h>

#define QLEN 	        10
#define MY_PORT_ID 	8000
#define MAXLINE 	256
#define MAXSIZE 	512   

#define ACK                   	2
#define NACK                  	3
#define REQUESTFILE           	100
#define STOREFILE 	      	200
#define MKDIR                   300
#define COMMANDNOTSUPPORTED   	150
#define COMMANDSUPPORTED      	160
#define BADFILENAME           	200
#define FILENAMEOK            	400
#define STARTTRANSFER         	500

#define SUCCESS	                250
#define NO_USER                 260
#define INVALID_PASS            270
#define LOGIN_SUCCESS           280

int writen(int sd,char *ptr,int size);
int readn(int sd,char *ptr,int size);
void *doftp(void *);
void recvfile(int);
void sendfile(int);
void auth_user(int ssock);

struct db_entry
{
	char *username;          // username
	char *password;          // password
	char *home_dir;          // home directory
	unsigned short user_id;  // user ID
	unsigned short group_id; // group ID
	struct db_entry *next;   // pointer to next user
};

int main()  
{
	int msock, ssock, pid, clilen, cmd;
	struct sockaddr_in client_addr;   
	char *service = "ftp";
	pthread_t th;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	msock = passiveTCP(service, QLEN);

	clilen = sizeof(client_addr);

	while(1) 
	{ 
		/* ACCEPT A CONNECTION AND THEN CREATE A CHILD TO DO THE WORK */
		/* LOOP BACK AND WAIT FOR ANOTHER CONNECTION                  */
		printf("server: starting accept\n");
		ssock = accept(msock ,(struct sockaddr *) &client_addr, &clilen);
		if (ssock < 0)
		{
			printf("server: accept error: %d\n", errno); 
			exit(1); 
		}
		if (pthread_create(&th, &attr, doftp, (void *)(long)ssock))
		{	
			printf("server: pthread_create error: %s\n", strerror(errno));
			exit(1);
		}
	}
}   

int passiveTCP(char *service, int qlen)
{
	struct sockaddr_in server;
	struct servent *serv;
	int sockfd, optval = 1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		printf("server: error setting SO_REUSEADDR socket option: %s\n", strerror(errno));
		exit(1);
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	if(serv = getservbyname(service, "tcp"))
		server.sin_port = serv->s_port;
	else
	{
		printf("server: getservbyname couldn't find %s\n", service);
		exit(1);
	}

	if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) <0)
	{
		printf("server: bind error: %s\n", strerror(errno));
		exit(1);
	}
	else
		printf("server: bind was successful\n");

	listen(sockfd,qlen);
	return sockfd;
}

void *doftp(void *sd)
{
	int req, msg_ok;
	long ssock = (long)sd;

	auth_user(ssock);

	while(1)
	{
		req = 0;

		if((readn(ssock,(char *)&req,sizeof(req))) < 0)
		{
			printf("server: read error %d\n",errno);
			exit(0);
		}

		req = ntohs(req);
		switch(req)
		{
			case STOREFILE:
				sendfile(ssock);
				break;
			case REQUESTFILE:
				recvfile(ssock);
				break;
			default:
				printf("server: command not supported\n");
				msg_ok = COMMANDNOTSUPPORTED;
				msg_ok = htons(msg_ok);
				if((writen(ssock,(char *)&msg_ok,sizeof(msg_ok))) < 0)
				{
					printf("server: write error :%d\n",errno);
					exit(0);
				}
		}
	}
}

void findUser(const char *user, struct db_entry *p_entry)
{
	FILE *fp = fopen("db.txt", "rb");
	char line[MAXLINE];
	char *pchar;

	if (fp == NULL)
	{
		printf("server: error opening file: %s\n", strerror(errno));
		exit(1);
	}

	memset(line, 0, MAXLINE);

	while(fgets(line, MAXLINE, fp) != NULL)
	{
		pchar = strtok(line, ":\n");

		if (!strcmp(pchar, user))
		{
			p_entry->username = (char *)malloc(sizeof(char) * MAXLINE);
			strcpy(p_entry->username, pchar);
			p_entry->password = (char *)malloc(sizeof(char) * MAXLINE);
			strcpy(p_entry->password, strtok(NULL, ":\n"));
			p_entry->home_dir = (char *)malloc(sizeof(char) * MAXLINE);
			strcpy(p_entry->home_dir, strtok(NULL, ":\n"));
			pchar = strtok(NULL, ":\n");
			if (pchar)
				p_entry->user_id = (unsigned short)(*pchar);
			pchar = strtok(NULL, ":\n");
			if (pchar)
				p_entry->group_id = (unsigned short)(*pchar);
			return;
		}
	}
	return;
}
			
void auth_user(int ssock)
{
	char uname[MAXLINE] = "";
	char pwd[MAXLINE] = "";
	int fail, tmp;
	struct db_entry p_entry;

	if((read(ssock,uname, MAXLINE)) < 0) 
	{
		printf("server: username read error :%d\n",errno);
		exit(1);
	}

	printf("server: username received is %s\n",uname);
	tmp = htons(SUCCESS);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		exit(0);   
	}

	if((read(ssock,pwd, MAXLINE)) < 0) 
	{
		printf("server: pwd read error :%d\n",errno);
		exit(1);
	}
	printf("server: pwd is %s\n",pwd);

	//crypted = crypt(pwd, salt);

	tmp = htons(SUCCESS);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		exit(0);   
	}

	//inputF = fopen("db.txt", "rb");

	// check opening correctness
	//if(inputF == NULL) 
	//       fprintf(stderr, "error during opening filename db.txt\n");

	//p_entry = (struct db_entry *)malloc(sizeof(struct db_entry));
	findUser(uname, &p_entry);

	if (p_entry.username == NULL)
	{
		tmp = htons(NO_USER);
		printf("server: Invalid username. Closing client connection.\n");
		if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			exit(0); 
		}
	}
	else if (strcmp(pwd, p_entry.password))
	{
		tmp = htons(INVALID_PASS);
		printf("server: Invalid password. Closing client connection.\n");
		if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			exit(0); 
		}
	}
	else
	{
		tmp = htons(LOGIN_SUCCESS);
		printf("server: Login successful.\n");
		if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			exit(1);
		}
	}
}

void sendfile(int ssock)
{
	int  newsockid,i,getfile,ack,msg,msg_2,c,len,n,msg_ok,fail,tmp,req;
	int no_writen,start_xfer, num_blks,num_last_blk;
	struct sockaddr_in my_addr, server_addr; 
	char fname[MAXLINE] = "";
	FILE *fp; 
	char in_buf[MAXSIZE];
	no_writen = 0;
	num_blks = 0;
	num_last_blk = 0;
	msg_ok = COMMANDSUPPORTED; 
	msg_ok = htons(msg_ok);
	if((writen(ssock,(char *)&msg_ok,sizeof(msg_ok))) < 0)
	{
		printf("server: write error :%d\n",errno);
		exit(0);
	}
	///fflush(fname);
	fail = FILENAMEOK;
	n = read(ssock,fname,sizeof(fname));
	fname[n]='\0';
	if(n < 0){
		printf("server: filename read error :%d\n",errno);
		fail = BADFILENAME ;
	}
	if((fp = fopen(fname,"w")) == NULL)/*cant open file*/
		fail = BADFILENAME;

	tmp = htons(fail);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		exit(0);   
	}
	if(fail == BADFILENAME)
	{
		printf("server cant open file\n");
		close(ssock);
		exit(0);
	}
	printf("server: filename is %s\n",fname);
	req = 0;
	if ((readn(ssock,(char *)&req,sizeof(req))) < 0)
	{
		printf("server: read error :%d\n",errno);
		exit(0);
	}
	printf("server: start transfer command, %d, received\n", ntohs(req));

	if((readn(ssock,(char *)&num_blks,sizeof(num_blks))) < 0)
	{
		printf("server read error on nblocks :%d\n",errno);
		exit(0);
	}
	num_blks = ntohs(num_blks);
	printf("server: client sent: %d blocks in file\n",num_blks);
	ack = ACK;  
	ack = htons(ack);
	if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("server: ack write error :%d\n",errno);
		exit(0);
	}
	if((readn(ssock,(char *)&num_last_blk,sizeof(num_last_blk))) < 0){
		printf("server: read error :%d on nbytes\n",errno);
		exit(0);
	}
	num_last_blk = ntohs(num_last_blk);  
	printf("server: client responded: %d bytes last blk\n",num_last_blk);
	if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("server: ack write error :%d\n",errno);
		exit(0);
	}


	/* BEGIN READING BLOCKS BEING SENT BY CLIENT */
	printf("server: starting to get file contents\n");
	for(i= 0; i < num_blks; i ++) {
		if((readn(ssock,in_buf,MAXSIZE)) < 0)
		{
			printf("server: block error read: %d\n",errno);
			exit(0);
		}
		no_writen = fwrite(in_buf,sizeof(char),MAXSIZE,fp);
		if (no_writen == 0) 
		{
			printf("server: file write error\n");
			exit(0);
		}
		if (no_writen != MAXSIZE) 
		{
			printf("server: file write  error : no_writen is less\n");
			exit(0);
		}
		/* send an ACK for this block */
		if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("server: ack write  error :%d\n",errno);
			exit(0);
		}
		printf(" %d...",i);
	}


	/*IF THERE IS A LAST PARTIALLY FILLED BLOCK, READ IT */

	if (num_last_blk > 0) {
		printf("%d\n",num_blks);      
		if((readn(ssock,in_buf,num_last_blk)) < 0){
			printf("server: last block error read :%d\n",errno);
			exit(0);
		}
		no_writen = fwrite(in_buf,sizeof(char),num_last_blk,fp); 
		if (no_writen == 0) {
			printf("server: last block file write err :%d\n",errno);
			exit(0);
		}
		if (no_writen != num_last_blk) {
			printf("server: file write error : no_writen is less 2\n");
			exit(0);
		}
		if((writen(ssock,(char *)&ack,sizeof(ack))) < 0){
			printf("server:ack write  error  :%d\n",errno);
			exit(0);
		}
	}
	else printf("\n");


	/*FILE TRANSFER ENDS. CLIENT TERMINATES AFTER  CLOSING ALL ITS FILES
	  AND SOCKETS*/ 
	fclose(fp);
	printf("server: FILE TRANSFER COMPLETE\n");
	close(ssock);

}
     
//CHILD PROCEDURE, WHICH ACTUALLY DOES THE FILE TRANSFER */
void  recvfile(int ssock)
{       
	//	int ssock=*((int *)sd);
	int i,fsize,fd,msg_ok,fail,fail1,req,c,ack,n;
	int no_read ,num_blks , num_blks1,num_last_blk,num_last_blk1,tmp;
	char fname[MAXLINE] = "";
	char out_buf[MAXSIZE];
	FILE *fp;

	no_read = 0;
	num_blks = 0;
	num_last_blk = 0; 


	/* START SERVICING THE CLIENT */ 

	/* get command code from client.*/
	/* only one supported command: 100 -  get a file */
	/* reply to client: command OK  (code: 160) */
	msg_ok = COMMANDSUPPORTED; 
	msg_ok = htons(msg_ok);
	if((writen(ssock,(char *)&msg_ok,sizeof(msg_ok))) < 0){
		printf("server: write error :%d\n",errno);
		exit(0);
	}
	fail = FILENAMEOK;
	n = read(ssock,fname,sizeof(fname));
	if(n < 0){
		printf("server: filename read error :%d\n",errno);
		fail = BADFILENAME ;
	}

	/* IF SERVER CANT OPEN FILE THEN INFORM CLIENT OF THIS AND TERMINATE */
	if((fp = fopen(fname,"r")) == NULL)/*cant open file*/
		fail = BADFILENAME;

	tmp = htons(fail);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0){
		printf("server: write error :%d\n",errno);
		exit(0);
	}
	if(fail == BADFILENAME){
		printf("server cant open file\n");
		close(ssock);
		exit(0);
		// return NULL;
	}
	printf("server: filename is %s\n",fname);
	req = 0;
	if ((readn(ssock,(char *)&req,sizeof(req))) < 0){
		printf("server: read error :%d\n",errno);
		exit(0);
		//return NULL;
	}
	printf("server: start transfer command, %d, received\n", ntohs(req));
	/*SERVER GETS FILESIZE AND CALCULATES THE NUMBER OF BLOCKS OF 
	  SIZE = MAXSIZE IT WILL TAKE TO TRANSFER THE FILE. ALSO CALCULATE
	  NUMBER OF BYTES IN THE LAST PARTIALLY FILLED BLOCK IF ANY. 
	  SEND THIS INFO TO CLIENT, RECEIVING ACKS */
	printf("server: starting transfer\n");
	fsize = 0;ack = 0;   
	while ((c = getc(fp)) != EOF) {fsize++;}
	num_blks = fsize / MAXSIZE; 
	num_blks1 = htons(num_blks);
	num_last_blk = fsize % MAXSIZE; 
	num_last_blk1 = htons(num_last_blk);
	if((writen(ssock,(char *)&num_blks1,sizeof(num_blks1))) < 0){
		printf("server: write error :%d\n",errno);
		//return NULL;
		exit(0);
	}
	printf("server: told client there are %d blocks\n", num_blks);  
	if((readn(ssock,(char *)&ack,sizeof(ack))) < 0){
		printf("server: ack read error :%d\n",errno);
		exit(0);
		//return NULL;
	}          
	if (ntohs(ack) != ACK) {
		printf("client: ACK not received on file size\n");
		exit(0);
		//return NULL;
	}
	if((writen(ssock,(char *)&num_last_blk1,sizeof(num_last_blk1))) < 0){
		printf("server: write error :%d\n",errno);
		exit(0);//return NULL;
	}
	printf("server: told client %d bytes in last block\n", num_last_blk);  
	if((readn(ssock,(char *)&ack,sizeof(ack))) < 0){
		printf("server: ack read error :%d\n",errno);
		exit(0);//return NULL;
	}
	if (ntohs(ack) != ACK){
		printf("server: ACK not received on file size\n");
		exit(0);
		// return NULL;
	}
	rewind(fp);    
	/* ACTUAL FILE TRANSFER STARTS  BLOCK BY BLOCK*/       
	for(i= 0; i < num_blks; i ++) { 
		no_read = fread(out_buf,sizeof(char),MAXSIZE,fp);
		if (no_read == 0) {
			printf("server: file read error\n");
			exit(0);
		}
		if (no_read != MAXSIZE){
			printf("server: file read error : no_read is less\n");
			exit(0);
		}
		if((writen(ssock,out_buf,MAXSIZE)) < 0){
			printf("server: error sending block:%d\n",errno);
			exit(0);
		}
		if((readn(ssock,(char *)&ack,sizeof(ack))) < 0){
			printf("server: ack read  error :%d\n",errno);
			exit(0);
		}
		if (ntohs(ack) != ACK) {
			printf("server: ACK not received for block %d\n",i);
			exit(0);
			//return NULL;
		}
		printf(" %d...",i);
	}

	if (num_last_blk > 0) { 
		printf("%d\n",num_blks);
		no_read = fread(out_buf,sizeof(char),num_last_blk,fp); 
		if (no_read == 0) {
			printf("server: file read error\n");
			exit(0);
		}
		if (no_read != num_last_blk) {
			printf("server: file read error : no_read is less 2\n");
			exit(0);
		}
		if((writen(ssock,out_buf,num_last_blk)) < 0){
			printf("server: file transfer error %d\n",errno);
			exit(0);
		}
		if((readn(ssock,(char *)&ack,sizeof(ack))) < 0){
			printf("server: ack read  error %d\n",errno);
			exit(0);
		}
		if (ntohs(ack) != ACK) {
			printf("server: ACK not received last block\n");
			exit(0);
			// return NULL;
		}
	}
	else printf("\n");

	/* FILE TRANSFER ENDS */
	printf("server: FILE TRANSFER COMPLETE on socket %d\n",ssock);
	fclose(fp);
	close(ssock);

	//return NULL;
}


/*
  TO TAKE CARE OF THE POSSIBILITY OF BUFFER LIMMITS IN THE KERNEL FOR THE
 SOCKET BEING REACHED (WHICH MAY CAUSE READ OR WRITE TO RETURN FEWER CHARACTERS
  THAN REQUESTED), WE USE THE FOLLOWING TWO FUNCTIONS */  
   
int readn(int sd,char *ptr,int size)
{
	int no_left,no_read;
	no_left = size;
	while (no_left > 0) 
	{ no_read = read(sd,ptr,no_left);
		if(no_read <0)  return(no_read);
		if (no_read == 0) break;
		no_left -= no_read;
		ptr += no_read;
	}
	return(size - no_left);
}

int writen(int sd,char *ptr,int size)
{
	int no_left,no_written;
	no_left = size;
	while (no_left > 0) 
	{ no_written = write(sd,ptr,no_left);
		if(no_written <=0)  return(no_written);
		no_left -= no_written;
		ptr += no_written;
	}
	return(size - no_left);
}
