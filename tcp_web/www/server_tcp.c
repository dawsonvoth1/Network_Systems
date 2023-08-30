#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFSIZE 1000000 //1024

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
	char buf[BUFSIZE]; //=(char*)calloc(BUFSIZE, sizeof(char)); /* message buf */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */


	socklen_t addr_size;
	struct sockaddr_storage their_addr; //info about incoming connection
	int new_fd; //new socket

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
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
	memset((char *) &serveraddr, '\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); //netowrk byte order function
	serveraddr.sin_port = htons((unsigned short)portno); //netowrk byte order function

	/* 
	* bind: associate the parent socket with a port 
	*/
	if(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
		error("ERROR on binding");
	}

	clientlen = sizeof(clientaddr);

	if(listen(sockfd, 80) < 0){ //number of connections allowed in incoming queue
		perror("Listen error");

	}
	while (1) {
		// if(listen(sockfd, 80) < 0){ //number of connections allowed in incoming queue
		// 	perror("Listen error");
		// }
		addr_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockadrr *)&their_addr, &addr_size);
		memset(buf, '\0', BUFSIZE);
		n = recv(new_fd, buf, BUFSIZE, 0);
		if(n < 0){
			perror("Recieving error");
		}
		// printf("%s \n", buf);
		if(!fork()){
			close(sockfd);

			char method[10];
			char *uri=(char*)calloc(1024, sizeof(char));
			char version[BUFSIZE];
			memset(method, '\0', 10);
			memset(uri, '\0', 1024);
			memset(version, '\0', BUFSIZE);

			FILE* fo;

			struct stat _stat;

			sscanf(buf, "%s %s %s", method, uri, version);
			printf("%s %s %s %s\n", method, uri, version, strrchr(uri, '.'));

			char path[1024];
			memset(path, '\0', 1024);
			memset(path, '.', 1);
			memcpy(path+1, uri, strlen(uri));

			char status[50];
			memset(status, '\0', 50);

			if(strcmp(method, "GET")){
				memcpy(status, " 405 Method Not Allowed", 24);
			}
			else if(!!strcmp(version, "HTTP/1.1") && !!strcmp(version, "HTTP/1.0")){
				memcpy(status, " 505 HTTP Version Not Supported", 32);
			}
			else{
				printf("%s \n", path);
				fo = fopen(path, "rb");
				if(fo==NULL){
					if(errno == EACCES){
						memcpy(status, " 403 Forbidden", 15);
					}
					else if(errno == ENOENT){
						memcpy(status, " 404 Not Found", 15);
					}
					else{
						memcpy(status, " 400 Bad Request", 17);
					}
					perror("Open error");
				}
				else if(fstat(fileno(fo), &_stat) == -1){
					perror("fstat error");
					printf("%d", errno);
				}
				else if(S_ISDIR(_stat.st_mode)){
					struct dirent *curr;
					DIR *d = opendir(path);
					if (d == NULL) {
						perror("Error from perror");
					}
					else{
						int found=0;
						while ((curr = readdir(d)) != NULL && !found){
							if(!strcmp(curr->d_name, "index.html") || !strcmp(curr->d_name, "index.htm")){
								memcpy(path+strlen(path), curr->d_name, strlen(curr->d_name));
								memcpy(uri+strlen(uri), curr->d_name, strlen(curr->d_name));
								found=1;
							}
						}
						if(!found){
							memcpy(status, " 404 Not Found", 15);
						}
						else{
							fo = freopen(path, "rb", fo);
							if(fo==NULL){
								if(errno == EACCES){
								memcpy(status, " 403 Forbidden", 15);
								}
								else if(errno == ENOENT){
									memcpy(status, " 404 Not Found", 15);
								}
								else{
									memcpy(status, " 400 Bad Request", 17);
								}
								perror("freopen error");
							}
						}
						closedir(d);
					}
				}
			}

			

			//get length of file in bytes
			long int leng=0; //length of buffer to be sent
			if(status[0] == '\0'){
				fseek(fo,0,SEEK_END);
				leng=ftell(fo);
				// printf("%ld \n",leng);
				fseek(fo,0,SEEK_SET);
				//read file to buffer
				memset(buf, '\0', BUFSIZE);
				char tmp[BUFSIZE];
				memset(tmp, '\0', BUFSIZE);
				// memcpy(buf, tmp, strlen(tmp)); //19
				size_t s=fread(buf, 1, BUFSIZE, fo);
				// printf("%lu \n", s);
				// printf("%s \n", buf);
				if(s != leng){
					perror("fread perror");
				}
				if(feof(fo)==0){
					perror("end of file not set");
				}

				char *res=" 200 OK\r\n";
				memcpy(version+strlen(version), res, strlen(res));
				char *c_t="Content-Type: ";
				memcpy(version+strlen(version), c_t, strlen(c_t));
				// printf("%s\n", version);
				if(!strcmp(strrchr(uri, '.'), ".html")){
					char *ext="text/html\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else if(!strcmp(strrchr(uri, '.'), ".txt")){
					char *ext="text/plain\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else if(!strcmp(strrchr(uri, '.'), ".png")){
					char *ext="image/png\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else if(!strcmp(strrchr(uri, '.'), ".gif")){
					char *ext="image/gif\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else if(!strcmp(strrchr(uri, '.'), ".jpg")){
					char *ext="image/jpg\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else if(!strcmp(strrchr(uri, '.'), ".css")){
					char *ext="text/css\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else if(!strcmp(strrchr(uri, '.'), ".js")){
					char *ext="application/javascript\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				else{
					char *ext="\r\n";
					memcpy(version+strlen(version), ext, strlen(ext));
				}
				char *c_l="Content-Length: ";
				memcpy(version+strlen(version), c_l, strlen(c_l));
				char num[sizeof(long)*8+1];
				char *last_line="\r\n\r\n";
				long int tmp_leng=strlen(version)+strlen(last_line)+sizeof(num)+leng; //changed from tmp_leng
				sprintf(num, "%ld", tmp_leng);
				memcpy(version+strlen(version), num, strlen(num));
				printf("%s\n", version);
				memcpy(version+strlen(version), last_line, strlen(last_line));
				memcpy(version+strlen(version), buf, leng);
				leng=tmp_leng;
			}
			else{
				memcpy(version+strlen(version), status, strlen(status));
				char *c_t="\r\nContent-Type: ";
				memcpy(version+strlen(version), c_t, strlen(c_t));
				char *ext="text/html\r\n";
				memcpy(version+strlen(version), ext, strlen(ext));
				char *c_l="Content-Length: ";
				memcpy(version+strlen(version), c_l, strlen(c_l));
				char *last_line="\r\n\r\n";
				char num[sizeof(long)*8+1];
				leng = strlen(version)+strlen(last_line)+sizeof(num);
				sprintf(num, "%ld", leng);
				memcpy(version+strlen(version), num, strlen(num));
				memcpy(version+strlen(version), last_line, strlen(last_line));
				memcpy(version+strlen(version), version, strlen(version));
				leng=leng + strlen(version);
				printf("Response: %s\n length: %ld\n", version, leng);
			}
			
			long int count=0;
			printf("%ld \n", leng);
			size_t left=leng;
			while(leng > count){
				n = send(new_fd, version+count, 1024, 0);
				if(n < 0){
					perror("Sending error");
					break;
				}
				left=left-n;
				count=count+n;
				printf("%zu \n", left);
				printf("Socket %d has sent %ld out of %ld \n", new_fd, count, leng);
			}
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;
}