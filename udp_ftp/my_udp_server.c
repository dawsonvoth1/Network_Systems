/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */


//for stat
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 50000

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char **argv) {
	int sockfd; /* socket */
	int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	// char buf[BUFSIZE]; /* message buf */
	unsigned char *buf=(char*)calloc(BUFSIZE, sizeof(char));
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */


	char *filename=(char*)calloc(BUFSIZE, sizeof(char));
	unsigned char *buffer=(char*)calloc(BUFSIZE, sizeof(char));
	long int leng=0; //file length
	long int buffer_length=0; 

	/* 
	 * check command line arguments 
	 */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	/* 
	 * socket: create the parent socket 
	 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0){
		error("ERROR opening socket");
	}

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		error("ERROR on binding");
	}

	/* 
	 * main loop: wait for a datagram, then echo it
	 */
	clientlen = sizeof(clientaddr);
	while (1) {
		buffer_length=0;
		memset(buf, '\0', BUFSIZE);
		memset(buffer, '\0', BUFSIZE);
		memset(filename, '\0', BUFSIZE);
		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0){
			error("ERROR in recvfrom");
		}

		/* 
		 * gethostbyaddr: determine who sent the datagram
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL){
			error("ERROR on gethostbyaddr");
		}
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL){
			error("ERROR on inet_ntoa\n");
		}
		printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
		printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
		

		//GET filename
		if(buf[0]=='g' && buf[1]=='e' && buf[2]=='t' && buf[3]==' '){
			printf("GET called\n");
			char path[1024];
			getcwd(path, 1024);
			printf("%s \n", path);
			strcpy(filename, buf+4);
			printf("%s\n", filename);
			FILE* fo = fopen(filename, "rb");
			if(fo == NULL){
				printf("%d \n", errno);
				perror("Error printed by perror");
			}
			else{
				//get length of file in bytes
				leng=0;
				fseek(fo,0,SEEK_END);
				leng=ftell(fo);
				printf("%ld \n",leng);
				fseek(fo,0,SEEK_SET);
				//read file to buffer
				size_t s=fread(buffer, 1, BUFSIZE, fo);
				printf("%lu \n", s);
				if(s != leng){
					perror("fread perror");
				}
				if(feof(fo)!=0){
					perror("end of file");
				}
				buffer_length=leng;
				// printf("file contents: %s \n", buffer);
			}
			fclose(fo);
		}
		//PUT filename
		else if(buf[0]=='p' && buf[1]=='u' && buf[2]=='t' && buf[3]==' '){
			printf("PUT called \n");
			int i=0; //input length
			for(i=0; i<BUFSIZE; i++){
				if(buf[i]=='\0'){
					break;
				}
			}
			strcpy(filename, buf+4);
			printf("%s", filename);
			FILE* writeFile=fopen(filename, "wb");
			if(writeFile == NULL){
				char* res="Unsuccessfully PUT, couldn't write file \n";
				memcpy(buffer, res, strlen(res));
				buffer_length=strlen(res);
				printf("%d \n", errno);
				perror("Error printed by perror");
			}
			else{
				//send a number first to make sure whole file is writen?
				size_t x=fwrite(buf+i+1, 1, n-i-1, writeFile);
				if(x!=n-i-1){
					char* res="Unsuccessfully PUT, incorrect bytes writen \n";
					memcpy(buffer, res, strlen(res));
					buffer_length=strlen(res);
				  	perror("Error from perror");
				}
				else{
					//Send how many bits put
					char* res="Successfully PUT \n";
					memcpy(buffer, res, strlen(res));
					buffer_length=strlen(res);
					// memmove(buffer+strlen(res), &x, sizeof(size_t));
				}
			}
			fclose(writeFile);
		}
		//DELETE filename
		else if(buf[0]=='d' && buf[1]=='e' && buf[2]=='l' && buf[3]=='e' && buf[4]=='t' && buf[5]=='e' && buf[6]==' '){
			int x=remove(buf+7);
			if(x==0){
				char* res="Successfull DELETE \n";
				memcpy(buffer, res, strlen(res));
				buffer_length=strlen(res);
			}
			else{
				char* res="Unsuccessfully DELETE \n";
				memcpy(buffer, res, strlen(res));
				buffer_length=strlen(res);
				perror("Error from perror");
			}
		}
		//ls
		else if(buf[0]=='l' && buf[1]=='s' && buf[2]=='\0'){
			struct dirent *curr;
			DIR *d = opendir(".");
			if (d == NULL) {
				char* res="Unsuccessfully ls call \n";
				buffer_length=strlen(res);
				memcpy(buffer, res, buffer_length);
				perror("Error from perror");
			}
			else{
				while ((curr = readdir(d)) != NULL){
					strcat(filename, curr->d_name);
					strcat(filename, "\n");
					// printf("%s\n", curr->d_name);
				}
				closedir(d);
				buffer_length=strlen(filename);
				memcpy(buffer, filename, buffer_length);
			}
		}
		//EXIT
		else if(buf[0]=='e' && buf[1]=='x' && buf[2]=='i' && buf[3]=='t' && buf[4]=='\0'){
			char* res="Goodbye client.\n";
			buffer_length=strlen(res);
			memcpy(buffer, res, buffer_length);
		}
		//other
		else{
			// sleep(10); //testing timeout functionality
			char* res="Incorrect command\nEnter: get <filename>, put <filename>, delete <filename>, ls, or exit\nCapitals and spaces do matter\n";
			buffer_length=strlen(res);
			memcpy(buffer, res, buffer_length);
		}
		/* 
		 * sendto: echo the input back to the client 
		 */
		n = sendto(sockfd, buffer, buffer_length, 0, (struct sockaddr *) &clientaddr, clientlen);
		if (n < 0) {
			error("ERROR in sendto");
		}
	}
}
