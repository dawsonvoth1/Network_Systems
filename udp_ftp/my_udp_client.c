/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define BUFSIZE 50000


static void sig_alarm(int sig){
	printf("in sig handler of alarm \n");
	return;
}

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
		perror(msg);
		exit(0);
}


int main(int argc, char **argv) {
	int sockfd, portno, n;
	int serverlen;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	unsigned char *buf=(char*)calloc(BUFSIZE, sizeof(char));
	unsigned char *recBuf=(char*)calloc(BUFSIZE, sizeof(char));
	unsigned char *sendBuf=(char*)calloc(BUFSIZE, sizeof(char));
	char *filename=(char*)calloc(BUFSIZE, sizeof(char));
	size_t s; //for fread
	long int leng=0; //for file length
	long int buf_length;
	int command_call=0; //to keep track of logic of different calls

	/* check command line arguments */
	if (argc != 3) {
		fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	hostname = argv[1];
	portno = atoi(argv[2]);

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		exit(0);
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(portno);


	//struct to set socket options timeout on recfrom
	struct timeval t;
	t.tv_sec=2;
	t.tv_usec=0;

	while(1){

		command_call=0;
		int o=setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);
		if(o<0){
			perror("socket options error");
		}

		/* get a message from the user */
		memset(buf, '\0', BUFSIZE);
		printf("Please enter msg: ");
		int c; //char input
		int k=0; //input length
		while(1){
			c=getchar();
			if(c == '\n' || c == '\0'){
				buf[k]='\0';
				break;
			}
			else{
				buf[k]=c;
				k++;
			}
		}

		//GET == 1
		if(buf[0]=='g' && buf[1]=='e' && buf[2]=='t' && buf[3]==' '){
			command_call=1;
			buf_length=k+1;
		}
		//PUT == 2
		else if(buf[0]=='p' && buf[1]=='u' && buf[2]=='t' && buf[3]==' '){
			command_call=2;
			memset(sendBuf, '\0', BUFSIZE);
			memcpy(filename, buf+4, k-4);
			FILE* fput = fopen(filename, "rb");
			//get file length and set position back to start after counting
			leng=0;
			fseek(fput,0,SEEK_END);
			leng=ftell(fput);
			fseek(fput,0,SEEK_SET);
			if(fput == NULL){
				printf("%d \n", errno);
				perror("Error printed by perror");
			}
			else{
				s=0;
				s=fread(sendBuf, 1, BUFSIZE, fput);
				if(s != leng){
					perror("fread perror");
				}
				if(feof(fput)!=0){
					perror("end of file");
				}
			}
			fclose(fput);
			//what if bigger then BUFSIZE?
			memmove(buf+k+1, sendBuf, leng);
			buf_length=leng+k+1;
			if(buf_length > BUFSIZE){
				printf("packet size is too big data will be cut off\n");
				buf_length=BUFSIZE;
			}
			// printf("%s \n", buf);
		}
		//DELETE == 3
		else if(buf[0]=='d' && buf[1]=='e' && buf[2]=='l' && buf[3]=='e' && buf[4]=='t' && buf[5]=='e' && buf[6]==' '){
			command_call=3;
			buf_length=k+1;
		}
		//ls == 4
		else if(buf[0]=='l' && buf[1]=='s' && buf[2]=='\0'){
			command_call=4;
			buf_length=k+1;
		}
		//EXIT == 5
		else if(buf[0]=='e' && buf[1]=='x' && buf[2]=='i' && buf[3]=='t' && buf[4]=='\0'){
			command_call=5;
			buf_length=k+1;
		}
		else{
			buf_length=k+1;
		}


		/* send the message to the server */
		serverlen = sizeof(serveraddr);
		n = sendto(sockfd, buf, buf_length, 0, (struct sockaddr *) &serveraddr, serverlen);
		if (n < 0) {
			perror("ERROR in sendto");
		}
		
		/* print the server's reply */
		memset(recBuf, '\0', BUFSIZE);
		// alarm(2);
		n = recvfrom(sockfd, recBuf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
		if (n < 0){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				printf("Request timed out\n");
			}
			else{
				perror("ERROR in recvfrom");
			}
		}
		else if(command_call==1){
			FILE* writeFile=fopen(buf+4, "wb");
			if(writeFile == NULL){
				printf("%d \n", errno);
				perror("Error printed by perror");
			}
			else{
				//send a number first to make sure whole file is writen?
				size_t x=fwrite(recBuf, 1, n, writeFile);
				if(x!=n){
					printf("Wrote %ld bytes from GET, did not write all bytes recieved \n", x);
				  	perror("Error from perror");
				}
				else{
					printf("Wrote %ld bytes from GET\n", x);
				}
			}
			fclose(writeFile);
		}
		else if(command_call==2){
			//compare bits sent?
			printf("%s", recBuf);
		}
		else if(command_call==3){
			printf("%s", recBuf);
		}
		else if(command_call==4){
			printf("%s", recBuf);
		}
		else if(command_call==5){
			printf("%s", recBuf);
			break;
		}
		else{
			printf("%s", recBuf);
		}

		if(n==BUFSIZE){
			printf("packet recieved was the max size and therefore some may not have been sent\n");
		}



	}//end of while loop

	// free(buf);
	// free(filename);
	// free(recBuf);
	// free(sendBuf);
	close(sockfd);

	return 0;
}
