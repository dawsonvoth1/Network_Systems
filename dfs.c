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

#include <openssl/md5.h>
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

int server_put(char *buf, char* dir, int n){
	char header[210];
	memset(header, '\0', 210);
	memcpy(header, buf, 200);
	char *method;
	char *filename;
	struct timeval curr_t;
	char* delim="\r\n\r\n";
	method=strtok(header, delim);
	filename=strtok(NULL, delim);
	printf("%s,%s,\n", method, filename);
	int location_in_buf=strlen(method)+strlen(filename)+strlen(delim)+strlen(delim);
	memcpy(&curr_t, buf + location_in_buf, sizeof(curr_t));
	printf("%ld, %ld,", curr_t.tv_sec, curr_t.tv_usec);
	
	//md5 hash filename
	unsigned long hash_n=strlen(filename);
	unsigned char md[MD5_DIGEST_LENGTH];
	unsigned char* res=MD5(filename, hash_n, md);

	char string_md[MD5_DIGEST_LENGTH*2 + 1];
	for(int j = 0; j < MD5_DIGEST_LENGTH; j++){
		sprintf(string_md+(j*2), "%02x", md[j]);
	}
	char hashed_file_location[50];
	memset(hashed_file_location, '\0', 50);
	// strcat(hashed_file_location, "./");
	strcat(hashed_file_location, dir);
	strcat(hashed_file_location, "/");
	strcat(hashed_file_location, string_md);
	printf("%s in put\n", hashed_file_location);

	int rf_put_first=0;
	FILE* readFile=fopen(hashed_file_location, "r");
	if(readFile == NULL){
		//file doesn't exist or error
		perror("try to read file if there error");
	}
	else{
		// file does exist already
		// compare timestamps
		char *rf_buf=(char*)calloc(BUFSIZE, sizeof(char));
		memset(rf_buf, '\0', BUFSIZE);
		if(fread(rf_buf, BUFSIZE, 1, readFile) > 0){
			char *rf_filename;
			struct timeval rf_t;
			rf_filename=strtok(rf_buf, delim);
			if(strcmp(rf_filename, filename) != 0){
				printf("error: file in hash doesn't have same name");
			}
			else{
				memcpy(&rf_t, rf_buf + strlen(rf_filename) + strlen(delim), sizeof(struct timeval));
				// printf("%ld, %ld,", rf_t.tv_sec, rf_t.tv_usec);
				// long difsec=(rf_buf).tv_sec - curr_t.tv_sec;
				if(rf_t.tv_sec > curr_t.tv_sec){
					//read file was put first
					rf_put_first = 1;
				}
				else if(rf_t.tv_sec == curr_t.tv_sec && rf_t.tv_usec > curr_t.tv_usec){
					rf_put_first = 1;
				}
			}
		}
		fclose(readFile);
	}
	if(rf_put_first != 1){
		FILE* writeFile=fopen(hashed_file_location, "wb+");
		if(writeFile == NULL){
			perror("put write file error");
			return -1;
		}
		else{
			int take_off_method=strlen(method) + strlen(delim);
			// printf("%d\n", sizeof(buf));
			if(fwrite(buf + take_off_method, 1, n - take_off_method, writeFile) != n - take_off_method){
				perror("writing file in put error");
				return -1;
			}
			else{
				char list_location[30];
				memset(list_location, '\0', 30);
				// strcat(list_location, "./");
				strcat(list_location, dir);
				strcat(list_location, "/list.txt");
				printf("list location in put %s\n", list_location);
				FILE* listFile=fopen(list_location, "a+");
				if(listFile == NULL){
					perror("list file open error");
				}
				else{
					int exists=0;
					char *line=NULL;
					size_t len=0;
					size_t r=0;
					char *curr_name=(char*)calloc(100, sizeof(char));
					while((r=getline(&line, &len, listFile)) != -1 && exists != 1){
						memset(curr_name, '\0', 100);
						memcpy(curr_name, line, r-1);
						if(strcmp(filename, curr_name) == 0){
							exists=1;
						}
					}
					if(exists != 1){
						char add_filename[101];
						strcat(add_filename, filename);
						strcat(add_filename, "\n");
						if(fwrite(add_filename, 1, strlen(add_filename), listFile) != strlen(add_filename)){
							perror("writing list file error");
						}
					}
					fclose(listFile);
				}
			}
			fclose(writeFile);
		}
	}
    return 0;
}








int server_delete(char *filename, char* dir){
	
	//md5 hash filename
	unsigned long n=strlen(filename);
	unsigned char md[MD5_DIGEST_LENGTH];
	unsigned char* res=MD5(filename, n, md);

	char string_md[MD5_DIGEST_LENGTH*2 + 1];
	for(int j = 0; j < MD5_DIGEST_LENGTH; j++){
		sprintf(string_md+(j*2), "%02x", md[j]);
	}
	char hashed_file_location[50];
	memset(hashed_file_location, '\0', 50);
	strcat(hashed_file_location, dir);
	strcat(hashed_file_location, "/");
	strcat(hashed_file_location, string_md);
	printf("%s %s in delete\n",filename, hashed_file_location);

	if(remove(hashed_file_location) != 0){
		perror("error removing file %s\n");
		return -1;
	}

	char list_location[30];
	memset(list_location, '\0', 30);
	strcat(list_location, dir);
	strcat(list_location, "/list.txt");

	char temp_list_location[30];
	memset(temp_list_location, '\0', 30);
	strcat(temp_list_location, dir);
	strcat(temp_list_location, "/temp_list.txt");

	FILE* listFile=fopen(list_location, "a+");
	FILE* templistFile=fopen(temp_list_location, "w+");
	if(listFile == NULL){
		perror("list file open error");
	}
	else if(templistFile == NULL){
		perror("temp_list file open error");
	}
	else{
		int exists=0;
		char *line=NULL;
		size_t len=0;
		size_t r=0;
		char *curr_name=(char*)calloc(100, sizeof(char));
		while((r=getline(&line, &len, listFile)) != -1 && exists != 1){
			memset(curr_name, '\0', 100);
			memcpy(curr_name, line, r-1);
			if(strcmp(filename, curr_name) == 0){
				exists=1;
			}
			else{
				if(fwrite(curr_name, strlen(curr_name), 1, templistFile) != strlen(curr_name)){
					perror("write temp_list error");
				}
			}
		}
		fclose(templistFile);
		fclose(listFile);
		if(remove(list_location) != 0){
			perror("error removing list");
			return -1;
		}
		else if(rename(temp_list_location, list_location) != 0){
			perror("error renaming temp_list");
			return -1;
		}
	}
    return 0;
}










int server_get(char* filename, char* dir, char* response, long *res_size){
	//md5 hash filename
	unsigned long n=strlen(filename);
	unsigned char md[MD5_DIGEST_LENGTH];
	unsigned char* res=MD5(filename, n, md);

	char string_md[MD5_DIGEST_LENGTH*2 + 1];
	for(int j = 0; j < MD5_DIGEST_LENGTH; j++){
		sprintf(string_md+(j*2), "%02x", md[j]);
	}
	char hashed_file_location[50];
	memset(hashed_file_location, '\0', 50);
	strcat(hashed_file_location, dir);
	strcat(hashed_file_location, "/");
	strcat(hashed_file_location, string_md);
	printf("%s %s in get\n",filename, hashed_file_location);

	memset(response, '\0', BUFSIZE);

	FILE* readFile=fopen(hashed_file_location, "rb");
	if(readFile == NULL){
		perror("get file open error");
		return -1;
	}
	else{
		char *line=NULL;
		size_t len=0;
		size_t r=0;
		long curr_pos=0;
		while((r=getdelim(&line, &len, '\r', readFile)) != -1){
			memcpy(response + curr_pos, line, r);
			curr_pos = curr_pos +r;
		}
		fclose(readFile);
		memcpy(res_size, &curr_pos, sizeof(long));
		printf("%ld\n", curr_pos);
	}
	return 0;
}








int main(int argc, char **argv) {
	int sockfd; /* socket */
	int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char *buf=(char*)calloc(BUFSIZE, sizeof(char)); /* message buf */
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
		}
		if(!fork()){
			printf("n: %d\n", n);

			char response[BUFSIZE];
			memset(response, '\0', BUFSIZE);
			long size_of_res=0;

			char method[10];
			char* delim="\r\n\r\n";

			sscanf(buf, "%s", method);


			if(method == NULL){
				printf("error: no command\n");
			}
			else if(strcmp(method, "List") == 0){
				//do list
			}
			else if(strcmp(method, "Get") == 0){
				//do get
				char* method=strtok(buf, delim);
				char* filename=strtok(NULL, delim);
				long *res_size=(long *)calloc(1, sizeof(long));
				memset(res_size, '\0', sizeof(long));
				if(server_get(filename, dir, response, res_size) ==0){
					size_of_res=*res_size;
				}
				else{
					memset(response, '\0', BUFSIZE);
					strcpy(response, "failed get");
					size_of_res=strlen(response);
				}



				// printf("%ld\n", *res_size);
				// char temp_list_location[30];
				// memset(temp_list_location, '\0', 30);
				// strcat(temp_list_location, dir);
				// strcat(temp_list_location, "/");
				// strcat(temp_list_location, filename);
				// FILE* t=fopen(temp_list_location, "wb+");
				// fwrite(response, 1, *res_size, t);
				// fclose(t);




			}
			else if(strcmp(method, "Put") == 0){
				//do put
				if(server_put(buf, dir, n) == 0){
					strcpy(response, "success");
					size_of_res=strlen(response);
				}
				else{
					strcpy(response, "failed");
					size_of_res=strlen(response);
				}
			}
			else if(strcmp(method, "Delete") == 0){
				char* method=strtok(buf, delim);
				char* filename=strtok(NULL, delim);
				if(server_delete(filename, dir) ==0){
					size_of_res=strlen(response);
				}
				else{
					strcpy(response, "failed edlete");
					size_of_res=strlen(response);
				}
			}
			else{
				printf("error: incorrect command");
			}

			if(send(new_fd, response, size_of_res, 0) < 0){
				perror("error sending response");
			}



			
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;

}