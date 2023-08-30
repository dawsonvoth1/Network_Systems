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

// #include <openssl/md5.h>
#include <sys/time.h>

#define BUFSIZE 600000 //1024
#define CHUNKSIZE 250000

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

int server_put(char* buf, char* dir){
    return 0;
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

    char* dir=argv[1];
    if(mkdir(dir, S_IRWXU) < 0){
        perror("make directory error");
    }

	socklen_t addr_size;
	struct sockaddr_storage their_addr; //info about incoming connection
	int new_fd; //new socket

	/* 
	* check command line arguments 
	*/
	if (argc != 3) {
		fprintf(stderr, "usage: %s <folder> <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[2]);

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
        exit(-1);

	}
	
	while (1) {
		addr_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockadrr *)&their_addr, &addr_size);
		memset(buf, '\0', BUFSIZE);
		n = recv(new_fd, buf, BUFSIZE, 0);
		if(n < 0){
			perror("Recieving error");
            exit(-1);
		}
		printf("%s \n", buf);
		if(!fork()){
			close(sockfd);




			char *method;
            char *filename;
            struct timeval curr_t;
            char* delim="\r\n\r\n";
            method=strtok(buf, delim);
            filename=strtok(NULL, delim);
            printf("%s,%s,\n", method, filename);
            int location_in_buf=strlen(method)+strlen(filename)+strlen(delim)+strlen(delim);
            memcpy(&curr_t, buf + location_in_buf, sizeof(curr_t));
            printf("%ld, %ld,", curr_t.tv_sec, curr_t.tv_usec);
            
			//md5 hash filename
			unsigned long n=strlen(filename);
			unsigned char md[MD5_DIGEST_LENGTH];
			unsigned char* res=MD5(filename, n, md);

			char hashed_file_location[50];
			strcat(hashed_file_location, dir);
			strcat(hashed_file_location, "/");
			strcat(hashed_file_location, md);

			FILE* readFile=fopen(hashed_file_location, "rb");
			int rf_put_first=0;
			if(readFile != NULL){
				// file does exist already
				// compare timestamps
				char rf_buf=(char*)calloc(BUFSIZE, sizeof(char));
				memset(rf_buf, '\0', BUFSIZE);
				if(fread(rf_buf, BUFSIZE, 1, readFile) > 0){
					char rf_filename;
					struct timeval rf_t;
					rf_filename=strtok(rf_buf, delim);
					memcpy(&rf_t, rf_buf + strlen(rf_filename) + strlen(delim), sizeof(struct timeval));
					// printf("%ld, %ld,", rf_t.tv_sec, rf_t.tv_usec);
					long difsec=rf_buf.tv_sec - curr_t.tv_sec;
					if((rf_buf->tv_sec - curr_t.tv_sec > 0) && rf_buf->tv_usec - curr_t.tv_usec > 0){
						//read file was put first
						rf_put_first = 1;
					}
				}
				fclose(readFile);
			}
			if(rf_put_first != 1){
				FILE* writeFile=fopen(hashed_file_location, "wb+");
				if(writeFile == NULL){
					perror("put write file error");
				}
				else{
					int take_off_method=strlen(method) + strlen(delim);
					if(fwrite(buf + take_off_method, 1, sizeof(buf) - take_off_method, writeFile) != sizeof(buf) - take_off_method){
						perror("writing file in put error");
					}
					else{
						char list_location[30];
						strcat(list_location, dir);
						strcat(list_location, "/list.txt");
						FILE* listFile=fopen(list_location, "a+");
						if(listFile == NULL){
							perror("list file open error");
						}
						else{
							int exists=0;
							while(!feof(listFile)){
								char curr_name[100];
								fgets(curr_name, 100, listFile);
								if(strcmp(filename, curr_name) == 0){
									exists=1;
									break;
								}
							}
							if(exists != 1){
								char add_filename[101];
								strcat(add_filename, filename);
								strcat(add_filename, "\n");
								if(fwrite(add_filename, strlen(add_filename), 1, listFile) != strlen(add_filename)){
									perror("writing list file error");
								}
							}
							fclose(listFile);
						}
					}
					fclose(writeFile);
				}
			}
            






			
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;

}