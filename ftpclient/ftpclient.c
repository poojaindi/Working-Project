#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#define STR 200
#define RTR 300

#define MAXSIZE 		512
#define ACK                   	2
#define NACK                  	3
#define REQUESTFILE           	100
#define STOREFILE 	      	200
#define COMMANDNOTSUPPORTED   	150
#define COMMANDSUPPORTED      	160
#define BADFILENAME           	200
#define FILENAMEOK            	400
#define STARTTRANSFER         	500

#define SUCCESS	                250
#define NO_USER                 260
#define INVALID_PASS            270
#define LOGIN_SUCCESS           280

extern int errno;

int errexit(const char *format, ...);
int connectTCP(const char *hostname, const char *service);
int TCPecho(const char *hostname, const char *service);
int readn(int sd,char *ptr,int size);
int writen(int sd,char *ptr,int size);
void readfile(int,char *);
void storefile(int,char *);
void auth_user (int connectfd, char *user, char *pass);

#define LINELEN 4096

int main(int argc, char *argv[])
{
	char *hostname = "localhost"; //host to use if none supplied
	char *service = "ftp", filename[50] = "";    //defaulto service port
	char user[50] = "", pass[50] = "";
	int cmd, connectfd;
	/*User has to give hostname as a cmd line argument */

	if(argc==2)
		hostname = argv[1];
	else
	{
		fprintf(stderr, "usage: ./4_HW2_Q1_client.exe [hostname]\n");
		exit(1);
	}

	connectfd = connectTCP(hostname, service);

	printf("Authenticating...\n");
	printf("Enter user name: ");
	scanf("%s", user);
	printf("Enter password: ");
	scanf("%s", pass);

	auth_user (connectfd, user, pass);
	
	printf("Enter file name: ");
	scanf("%s", filename);

	printf("Commands supported.\n100:RETR\n200:STOR\nEnter command: ");
	scanf("%d", &cmd);

	switch(cmd)
	{
		case STOREFILE: 
			storefile(connectfd,filename);
			break;
		case REQUESTFILE: 
			readfile(connectfd,filename); 	
			break;
		default: 
			printf("RES 500 command not implemented!\n");	
	}

	exit(0);
}

void auth_user (int connectfd, char *user, char *pass)
{
	int len, msg;

	/* send username to server */
	printf("client: sending username\n");
	len = strlen(user);
	if ((writen(connectfd, user, len))< 0)
	{
		printf("client: write error :%d\n", errno); 
		exit(0);
	}

	msg = 0;
	if ((readn(connectfd, (char *)&msg, sizeof(msg)))< 0)
	{
		printf("client: read  error :%d\n", errno); 
		exit(0); 
	}   
	msg = ntohs(msg);
	if (msg == SUCCESS) 
		printf("client: server recd username.\n");

	//crypted = crypt(argv[2], salt);
	//if (!crypted)
	//	return -1;

	/* send pwd to server */
	printf("client: sending pwd\n");
	len = strlen(pass);
	if ((writen(connectfd, pass, len))< 0)
	{
		printf("client: write  error :%d\n", errno); 
		exit(0);
	}

	msg = 0;
	if ((readn(connectfd,(char *)&msg,sizeof(msg)))< 0)
	{
		printf("client: read  error :%d\n", errno); 
		exit(0); 
	}   
	msg = ntohs(msg);
	if (msg == SUCCESS)
		printf("client: server recd pwd.\n");

	msg = 0;
	if ((readn(connectfd,(char *)&msg,sizeof(msg)))< 0)
		{printf("client: read  error :%d\n", errno); exit(0); }
	if (ntohs(msg) == NO_USER)
	{
		printf("client: Invalid username %s\n", user);
		exit(0);
	}
	else if (ntohs(msg) == INVALID_PASS)
	{
		printf("client: Invalid password %s\n", pass);
		exit(0);
	}
	else if (ntohs(msg) == LOGIN_SUCCESS)
		printf("client: Login successful for %s\n", user);
}

void storefile(int newsd, char * filename)
{
	//	int newsd=*((int *)sd);
	int i,fsize,fd,msg_ok,msg,fail,fail1,req,c,ack,n,len,getfile,msg_2,start_xfer;
	int no_read ,num_blks , num_blks1,num_last_blk,num_last_blk1,tmp;
	char fname[LINELEN];
	char out_buf[MAXSIZE];
	FILE *fp;

	no_read = 0;
	num_blks = 0;
	num_last_blk = 0; 

	getfile = htons(STOREFILE);
	printf("client: sending command request to ftp server\n");
	if((writen(newsd,(char *)&getfile,sizeof(getfile))) < 0){
		printf("client: write  error :%d\n", errno); exit(0);
	} 

	/* want for go-ahead from server */
	msg = 0; 

	if((readn(newsd,(char *)&msg,sizeof(msg)))< 0){
		printf("client: read  error :%d\n", errno); exit(0); 
	}
	msg = ntohs(msg);   
	if (msg==COMMANDNOTSUPPORTED) {
		printf("client: server refused command. goodbye\n");
		exit(0);
	}
	else{
		printf("client: server replied %d, command supported\n",msg);
	}

	/* send file name to server */

	printf("client: sending filename\n");
	//	printf("POOJA: sending filename %s\n",filename);
	len = strlen(filename);
	//	printf("POOJA: len is %d\n",len);
	n =  write(newsd,filename,len);
	if(n < 0){
		printf("client: write  error :%d\n", errno); exit(0);}
	/* see if server replied that file name is OK */
	msg_2 = 0;
	if ((readn(newsd,(char *)&msg_2,sizeof(msg_2)))< 0){
		printf("client: read  error :%d\n", errno); exit(0); 
	}   
	msg_2 = ntohs(msg_2);
	if (msg_2 == BADFILENAME) {
		printf("client: server reported bad file name. goodbye.\n");
		exit(0);
	}
	else{
		printf("client: server replied %d, filename OK\n",msg_2);
	}


	/* CLIENT KNOWS SERVER HAS BEEN ABLE TO OPEN THE FILE IN READ 
	   MODE AND IS ASKING FOR GO-AHEAD*/
	/* CLIENT NOW OPENS A COPY OF THE FILE IN WRITE MODE AND SENDS 
	   THE GOAHEAD TO SERVER*/ 
	printf("client: sending start transfer command\n");
	start_xfer = STARTTRANSFER;
	start_xfer = htons(start_xfer);
	if ((writen(newsd,(char *)&start_xfer,sizeof(start_xfer)))< 0){
		printf("client: write  error :%d\n", errno);
		exit(0);
	}
	if ((fp = fopen(filename,"r")) == NULL){
		printf(" client: local open file error \n");
		exit(0);
	}

	fsize = 0;ack = 0;   
	while ((c = getc(fp)) != EOF) {fsize++;}
	num_blks = fsize / MAXSIZE; 
	num_blks1 = htons(num_blks);
	num_last_blk = fsize % MAXSIZE; 
	num_last_blk1 = htons(num_last_blk);
	if((writen(newsd,(char *)&num_blks1,sizeof(num_blks1))) < 0){
		printf("client: write error :%d\n",errno);
		//return NULL;
		exit(0);
	}
	printf("client: told server there are %d blocks\n", num_blks);  
	if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){
		printf("client: ack read error :%d\n",errno);
		exit(0);//return NULL; 
	}          
	if (ntohs(ack) != ACK) {
		printf("client: ACK not received on file size\n");
		exit(0);
		//return NULL;
	}
	if((writen(newsd,(char *)&num_last_blk1,sizeof(num_last_blk1))) < 0){
		printf("client: write error :%d\n",errno);
		exit(0);//return NULL;
	}
	printf("client: told server %d bytes in last block\n", num_last_blk);  
	if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){
		printf("client: ack read error :%d\n",errno);
		exit(0);//return NULL; 
	}
	if (ntohs(ack) != ACK) {
		printf("client: ACK not received on file size\n");
		exit(0);
		//return NULL;
	}
	rewind(fp);    


	/* ACTUAL FILE TRANSFER STARTS  BLOCK BY BLOCK*/       


	for(i= 0; i < num_blks; i ++) { 
		no_read = fread(out_buf,sizeof(char),MAXSIZE,fp);
		if (no_read == 0) {
			printf("client: file read error\n");
			exit(0);
		}
		if (no_read != MAXSIZE){
			printf("client: file read error : no_read is less\n");
			exit(0);
		}
		if((writen(newsd,out_buf,MAXSIZE)) < 0){
			printf("client: error sending block:%d\n",errno);
			exit(0);
		}
		if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){
			printf("client: ack read  error :%d\n",errno);
			exit(0);
		}
		if (ntohs(ack) != ACK) {
			printf("client: ACK not received for block %d\n",i);
			exit(0);
			//return NULL;
		}
		printf(" %d...",i);
	}

	if (num_last_blk > 0) { 
		printf("%d\n",num_blks);
		no_read = fread(out_buf,sizeof(char),num_last_blk,fp); 
		if (no_read == 0){
			printf("client: file read error\n");
			exit(0);
		}
		if (no_read != num_last_blk) {
			printf("client: file read error : no_read is less 2\n");
			exit(0);
		}
		if((writen(newsd,out_buf,num_last_blk)) < 0){
			printf("client: file transfer error %d\n",errno);
			exit(0);
		}
		if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){
			printf("client: ack read  error %d\n",errno);
			exit(0);
		}
		if (ntohs(ack) != ACK) {
			printf("client: ACK not received last block\n");
			exit(0);
			//return NULL;
		}
	}
	else printf("\n");

	/* FILE TRANSFER ENDS */
	printf("client: FILE TRANSFER COMPLETE on socket %d\n",newsd);
	fclose(fp);
	close(newsd);

	//return NULL;

}

void readfile(int sockid, char * filename)
{
	int  newsockid,i,getfile,ack,msg,msg_2,c,len,n;
	int no_writen,start_xfer, num_blks,num_last_blk;
	struct sockaddr_in my_addr, server_addr; 
	FILE *fp; 
	char in_buf[MAXSIZE];
	//if(argc != 2) {printf("error: usage : sftp filename\n"); exit(0);}
	no_writen = 0;
	num_blks = 0;
	num_last_blk = 0;
	//len = strlen(argv[1]);
	/* tell server that we want to get a file */

	getfile = htons(REQUESTFILE);
	printf("client: sending command request to ftp server\n");
	if((writen(sockid,(char *)&getfile,sizeof(getfile))) < 0){
		printf("client: write  error :%d\n", errno); exit(0);
	} 

	/* want for go-ahead from server */
	msg = 0; 

	if((readn(sockid,(char *)&msg,sizeof(msg)))< 0){
		printf("client: read  error :%d\n", errno); exit(0); 
	}
	msg = ntohs(msg);   
	if (msg==COMMANDNOTSUPPORTED) {
		printf("client: server refused command. goodbye\n");
		exit(0);
	}
	else{
		printf("client: server replied %d, command supported\n",msg);
	}

	/* send file name to server */

	printf("client: sending filename\n");
	//	printf("POOJA: sending filename %s\n",filename);
	len = strlen(filename);
	//	printf("POOJA: len is %d\n",len);
	n =  write(sockid,filename,len);
	if(n < 0){
		printf("client: write  error :%d\n", errno); exit(0);}
	/* see if server replied that file name is OK */
	msg_2 = 0;
	if ((readn(sockid,(char *)&msg_2,sizeof(msg_2)))< 0){
		printf("client: read  error :%d\n", errno); exit(0); 
	}   
	msg_2 = ntohs(msg_2);
	if (msg_2 == BADFILENAME) {
		printf("client: server reported bad file name. goodbye.\n");
		exit(0);
	}
	else{
		printf("client: server replied %d, filename OK\n",msg_2);
	}


	/* CLIENT KNOWS SERVER HAS BEEN ABLE TO OPEN THE FILE IN READ 
	   MODE AND IS ASKING FOR GO-AHEAD*/
	/* CLIENT NOW OPENS A COPY OF THE FILE IN WRITE MODE AND SENDS 
	   THE GOAHEAD TO SERVER*/ 
	printf("client: sending start transfer command\n");
	start_xfer = STARTTRANSFER;
	start_xfer = htons(start_xfer);
	if ((writen(sockid,(char *)&start_xfer,sizeof(start_xfer)))< 0){
		printf("client: write  error :%d\n", errno);
		exit(0);
	}
	if ((fp = fopen(filename,"w")) == NULL){
		printf(" client: local open file error \n");
		exit(0);
	}



	/*NOW THE CLIENT IS READING INFORMATION FROM THE SERVER REGARDING HOW MANY
	  FULL BLOCKS OF SIZE MAXSIZE IT CAN EXPECT. IT ALSO RECEIVES THE NUMBER
	  OF BYTES REMAINING IN THE LAST PARTIALLY FILLED BLOCK, IF ANY */  

	if((readn(sockid,(char *)&num_blks,sizeof(num_blks))) < 0){
		printf("client: read error on nblocks :%d\n",errno);
		exit(0);
	}
	num_blks = ntohs(num_blks);
	printf("client: server responded: %d blocks in file\n",num_blks);
	ack = ACK;  
	ack = htons(ack);
	if((writen(sockid,(char *)&ack,sizeof(ack))) < 0){
		printf("client: ack write error :%d\n",errno);exit(0);
	}


	if((readn(sockid,(char *)&num_last_blk,sizeof(num_last_blk))) < 0){
		printf("client: read error :%d on nbytes\n",errno);
		exit(0);
	}
	num_last_blk = ntohs(num_last_blk);  
	printf("client: server responded: %d bytes last blk\n",num_last_blk);
	if((writen(sockid,(char *)&ack,sizeof(ack))) < 0){
		printf("client: ack write error :%d\n",errno);
		exit(0);
	}


	/* BEGIN READING BLOCKS BEING SENT BY SERVER */
	printf("client: starting to get file contents\n");
	for(i= 0; i < num_blks; i ++) {
		if((readn(sockid,in_buf,MAXSIZE)) < 0){
			printf("client: block error read: %d\n",errno);
			exit(0);
		}
		no_writen = fwrite(in_buf,sizeof(char),MAXSIZE,fp);
		if (no_writen == 0) {
			printf("client: file write error\n");
			exit(0);
		}
		if (no_writen != MAXSIZE) {
			printf("client: file write  error : no_writen is less\n");
			exit(0);
		}
		/* send an ACK for this block */
		if((writen(sockid,(char *)&ack,sizeof(ack))) < 0){
			printf("client: ack write  error :%d\n",errno);
			exit(0);
		}
		printf(" %d...",i);
	}


	/*IF THERE IS A LAST PARTIALLY FILLED BLOCK, READ IT */

	if (num_last_blk > 0) {
		printf("%d\n",num_blks);      
		if((readn(sockid,in_buf,num_last_blk)) < 0){
			printf("client: last block error read :%d\n",errno);
			exit(0);
		}
		no_writen = fwrite(in_buf,sizeof(char),num_last_blk,fp); 
		if (no_writen == 0) {
			printf("client: last block file write err :%d\n",errno);
			exit(0);
		}
		if (no_writen != num_last_blk) {
			printf("client: file write error : no_writen is less 2\n");
			exit(0);
		}
		if((writen(sockid,(char *)&ack,sizeof(ack))) < 0){
			printf("client :ack write  error  :%d\n",errno);
			exit(0);
		}
	}
	else printf("\n");


	/*FILE TRANSFER ENDS. CLIENT TERMINATES AFTER  CLOSING ALL ITS FILES
	  AND SOCKETS*/ 
	fclose(fp);
	printf("client: FILE TRANSFER COMPLETE\n");
	close(sockid);

}

int readn(int sd,char *ptr,int size)
{         
	int no_left,no_read;
	no_left = size;
	while (no_left > 0)
	{
		no_read = read(sd,ptr,no_left);
		if(no_read <0){
			return(no_read);
		}
		if (no_read == 0){
			break;
		}
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
	{
		no_written = write(sd,ptr,no_left);
		if(no_written <=0){
			return(no_written);
		}
		no_left -= no_written;
		ptr += no_written;
	}
	return(size - no_left);
}
