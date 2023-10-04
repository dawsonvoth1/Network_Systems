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
#include <time.h>

// #include <curl/curl.h>

#define BUFSIZE 2000000 //1024

#include <assert.h>
#include <stdbool.h>

typedef struct url_parser_url {
	char *protocol;
	char *host;
	char *port;
	char *path;
	char *query_string;
	int host_exists;
	char *host_ip;
} url_parser_url_t;

void free_parsed_url(url_parser_url_t *url_parsed) {
	if (url_parsed->protocol)
		free(url_parsed->protocol);
	if (url_parsed->host)
		free(url_parsed->host);
	if (url_parsed->path)
		free(url_parsed->path);
	if (url_parsed->query_string)
		free(url_parsed->query_string);

	free(url_parsed);
}

int parse_url(char *url, bool verify_host, url_parser_url_t *parsed_url) {
	char *local_url = (char *) malloc(sizeof(char) * (strlen(url) + 1));
	char *token;
	char *token_host;
	char *host_port;
	char *host_ip;

	char *token_ptr;
	char *host_token_ptr;

	char *path = NULL;

	// Copy our string
	strcpy(local_url, url);

	token = strtok_r(local_url, ":", &token_ptr);
	parsed_url->protocol = (char *) malloc(sizeof(char) * strlen(token) + 1);
	strcpy(parsed_url->protocol, token);

	// Host:Port
	token = strtok_r(NULL, "/", &token_ptr);
	if (token) {
		host_port = (char *) malloc(sizeof(char) * (strlen(token) + 1));
		strcpy(host_port, token);
	} else {
		host_port = (char *) malloc(sizeof(char) * 1);
		strcpy(host_port, "");
	}

	token_host = strtok_r(host_port, ":", &host_token_ptr);
	parsed_url->host_ip = NULL;
	if (token_host) {
		parsed_url->host = (char *) malloc(sizeof(char) * strlen(token_host) + 1);
		strcpy(parsed_url->host, token_host);

		if (verify_host) {
			struct hostent *host;
			host = gethostbyname(parsed_url->host);
			if (host != NULL) {
				parsed_url->host_ip = inet_ntoa(* (struct in_addr *) host->h_addr);
				parsed_url->host_exists = 1;
			} else {
				parsed_url->host_exists = 0;
			}
		} else {
			parsed_url->host_exists = -1;
		}
	} else {
		parsed_url->host_exists = -1;
		parsed_url->host = NULL;
	}

	// Port
	token_host = strtok_r(NULL, ":", &host_token_ptr);

	if (token_host){
		parsed_url->port = (char *) malloc(sizeof(char) * strlen(token_host) + 1);
		strcpy(parsed_url->port, token_host);
	}
	else{
		parsed_url->port = (char *) malloc(sizeof(char) * 2 + 1);
		strcpy(parsed_url->port, "80");
	}
	token_host = strtok_r(NULL, ":", &host_token_ptr);
	assert(token_host == NULL);

	token = strtok_r(NULL, "?", &token_ptr);
	parsed_url->path = NULL;
	if (token) {
		path = (char *) realloc(path, sizeof(char) * (strlen(token) + 2));
		strcpy(path, "/");
		strcat(path, token);

		parsed_url->path = (char *) malloc(sizeof(char) * strlen(path) + 1);
		strncpy(parsed_url->path, path, strlen(path));

		free(path);
	} else {
		parsed_url->path = (char *) malloc(sizeof(char) * 2);
		strcpy(parsed_url->path, "/");
	}

	token = strtok_r(NULL, "?", &token_ptr);
	if (token) {
		parsed_url->query_string = (char *) malloc(sizeof(char) * (strlen(token) + 1));
		strncpy(parsed_url->query_string, token, strlen(token));
	} else {
		parsed_url->query_string = NULL;
	}

	token = strtok_r(NULL, "?", &token_ptr);
	assert(token == NULL);

	free(local_url);
	free(host_port);
	return 0;
}

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

unsigned long hash(unsigned char *str){
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}

// // scheme://hostname:port/path?query
// int parseURL(char* url, char **results){
// 	char scheme[6];
// 	char hostname[1024];
// 	char port[8];
// 	char path[1024];
// 	char query[1024];
// 	char* parse_url=calloc(strlen(url) +1, sizeof(char));

// 	memset(scheme, '\0', 6);
// 	memset(hostname, '\0', 1024);
// 	memset(port, '\0', 8);
// 	memset(path, '\0', 1024);
// 	memset(query, '\0', 1024);
// 	memset(parse_url, '\0', strlen(url) +1);

// 	char* temp;
// 	char* res;
// 	strcpy(parse_url, url);

// 	temp=strtok_r(parse_url, ':', &res);
// 	strcpy(scheme, temp);


// }

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

	int timeout;

	char method[10];
	char *uri=(char*)calloc(1024, sizeof(char));
	char version[50];
	memset(method, '\0', 10);
	memset(uri, '\0', 1024);
	memset(version, '\0', 50);

	char *hostname=(char*)calloc(1024, sizeof(char));
	memset(hostname, '\0', 1024);
	char *port=(char*)calloc(10, sizeof(char));
	memset(port, '\0', 10);
	char *path=(char*)calloc(1024, sizeof(char));
	memset(path, '\0', 1024);
	char *full_url=(char*)calloc(1024, sizeof(char));
	memset(full_url, '\0', 1024);
	char *query=(char*)calloc(1024, sizeof(char));
	memset(query, '\0', 1024);


	socklen_t addr_size;
	struct sockaddr_storage their_addr; //info about incoming connection
	int new_fd; //new socket

	/* 
	* check command line arguments 
	*/
	if (argc != 3) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	timeout= atoi(argv[2]);

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
		addr_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockadrr *)&their_addr, &addr_size);
		memset(buf, '\0', BUFSIZE);
		n = recv(new_fd, buf, BUFSIZE, 0);
		if(n < 0){
			perror("Recieving error");
		}

		if(!fork()){
			close(sockfd);

			// char method[10];
			// char *uri=(char*)calloc(1024, sizeof(char));
			// char version[50];
			memset(method, '\0', 10);
			memset(uri, '\0', 1024);
			memset(version, '\0', 50);

			// char *hostname=(char*)calloc(1024, sizeof(char));
			memset(hostname, '\0', 1024);
			// char *port=(char*)calloc(10, sizeof(char));
			memset(port, '\0', 10);
			// char *path=(char*)calloc(1024, sizeof(char));
			memset(path, '\0', 1024);
			// char *full_url=(char*)calloc(1024, sizeof(char));
			memset(full_url, '\0', 1024);
			// char *query=(char*)calloc(1024, sizeof(char));
			memset(query, '\0', 1024);

			sscanf(buf, "%s %s %s", method, uri, version);

			url_parser_url_t *url_parsed;
			strcpy(uri, full_url);
			parse_url(uri, false, url_parsed);
			strcpy(hostname, url_parsed->host);
			strcpy(port, url_parsed->port);
			strcpy(path, url_parsed->path);
			strcpy(query, url_parsed->query_string);

			// CURLUcode rc;
			// CURLU *url = curl_url();
			// rc = curl_url_set(url, CURLUPART_URL, uri, 0);
			// if(!rc) {
			// 	rc = curl_url_get(url, CURLUPART_HOST, &hostname, 0);
			// 	if(!rc) {
			// 		// printf("the host is %s\n", hostname);
			// 	}
			// 	rc = curl_url_get(url, CURLUPART_PORT, &port, CURLU_DEFAULT_PORT);
			// 	if(!rc) {
			// 		// printf("the port is %s\n", port);
			// 	}
			// 	rc = curl_url_get(url, CURLUPART_PATH, &path, 0);
			// 	if(!rc) {
			// 		// printf("the path is %s\n", path);
			// 	}
			// 	rc = curl_url_get(url, CURLUPART_QUERY, &query, 0);
			// 	if(!rc) {
			// 		// printf("the query is %s\n", query);
			// 	}
			// 	rc = curl_url_get(url, CURLUPART_URL, &full_url, 0);
			// 	if(!rc) {
			// 		// printf("the full url is %s\n", full_url);
			// 	}
			// 	curl_url_cleanup(url);
			// }

			FILE *block=fopen("blocklist", "r");
			if(block == NULL){
				perror("blocklist open");
				exit(-1);
			}

			char *line=NULL;
			size_t len=0;
			size_t r=0;
			char *temp=(char*)calloc(1024, sizeof(char));
			while((r=getline(&line, &len, block)) != -1){
				memset(temp, '\0', 1024);
				memcpy(temp, line, r-1);
				// printf("%s\n", temp);
				if(strcmp(temp, hostname) == 0){
					send(new_fd, "HTTP/1.0 403 Forbidden\r\n", 25, 0);
					// printf("Forbidden\n");
					fclose(block);
					close(new_fd);
					exit(-1);
				}
			}

			fclose(block);

			char *name_r=(char*)calloc(1024, sizeof(char));
			memset(name_r, '\0', 1024);
			unsigned long hash_r=hash(full_url);
			sprintf(name_r, "%lu", hash_r);
			FILE *curr=fopen(name_r, "r+");
			printf("%s :\n", name_r);
			struct stat curr_info;
			if(curr != NULL){
				if(fstat(fileno(curr), &curr_info) == 0){
					// printf("%ld %ld \n", curr_info.st_mtimespec.tv_sec, curr_info.st_mtimespec.tv_nsec);
					struct timespec ts;
					timespec_get(&ts, TIME_UTC);
					// printf("%ld %ld \n", ts.tv_sec, ts.tv_nsec);
					long dif=ts.tv_sec - curr_info.st_mtimespec.tv_sec;
					// printf("%ld\n", dif);
					if(dif < timeout){
						memset(buf, '\0', BUFSIZE);
						n=fread(buf, sizeof(char), BUFSIZE, curr);
						if(send(new_fd, buf, BUFSIZE, 0) < 0){
							perror("new_fd cache send");
						}
						// printf("In cache %s\n", name_r);
						close(new_fd);
						fclose(curr);
						exit(0);			
					}
				}
				fclose(curr);
			}
			printf("%s %s %s %s\n", method, uri, version, strrchr(uri, '.'));
			printf("%s %s %s \n", hostname, port, path);

			int status2;
			struct addrinfo hints;
			struct addrinfo *servinfo, *p;  // will point to the results

			memset(&hints, 0, sizeof hints); // make sure the struct is empty
			hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
			hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

			if(strcmp(method, "GET")){
				send(new_fd, "HTTP/1.0 400 Bad Request\r\n", 27, 0);
				close(new_fd);
				exit(-1);
			}
			else if(!!strcmp(version, "HTTP/1.1") && !!strcmp(version, "HTTP/1.0")){
				send(new_fd, "HTTP/1.0 400 Bad Request\r\n", 27, 0);
				close(new_fd);
				exit(-1);
			}
			else if ((status2 = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
				fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status2));
				send(new_fd, "HTTP/1.0 404 Not Found\r\n", 25, 0);
				close(new_fd);
				exit(-1);
			}

			int socket2;
			for(p = servinfo; p != NULL; p = p->ai_next){
				socket2=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
				if(socket2 < 0){
					perror("socket2");
					continue;
				}
				
				if(connect(socket2, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
					close(socket2);
					perror("connect");
					continue;
				}
			}

			freeaddrinfo(servinfo);

			if(send(socket2, buf, BUFSIZE, 0) < 0){
				perror("socket2 send");
			}
			memset(buf, '\0', BUFSIZE);
			if((n=recv(socket2, buf, BUFSIZE, MSG_WAITALL)) < 0){
				perror("socket2 recv");
			}

			if(query == NULL){
				// if not error
				int code;

				char res[50];

				sscanf(buf, "%s %d %s", version, &code, res);
				if(code == 200){
					char *name_w=(char*)calloc(1024, sizeof(char));
					memset(name_w, '\0', 1024);
					// unsigned long hash_w=hash(full_url);
					unsigned long hash_w=hash(uri);
					sprintf(name_w, "%lu", hash_w);
					FILE *w=fopen(name_w, "w+");
					struct stat w_info;
					if(w != NULL){
						fwrite(buf, sizeof(char), BUFSIZE, w);
						fclose(w);
					}
					else{
						perror("write file");
					}
				}
			}

			if(send(new_fd, buf, BUFSIZE, 0) < 0){
				perror("new_fd send");
			}
			close(socket2);
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;
}