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
#include <signal.h>

#define MAX_ARG_SIZE 1000
#define BUFSIZE 500000

void sig_alarm(int num){

}

unsigned long hash(unsigned char *str){
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}


int send_packet(char *packet, char* response, char* folder, char* ip, char* port, int size){

    int n; //message size
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *res;  // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    status=getaddrinfo(ip, port, &hints, &servinfo);

    int sockfd;

    struct timeval t;
	t.tv_sec=2;
	t.tv_usec=0;
    int opt=setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);

    // for(res = servinfo; res != NULL; res = res->ai_next){
        sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if(sockfd < 0){
            perror("socket error");
            return -1;
        }

        //connect in one second or fail
        alarm(1);
        int c=connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
        alarm(0);

        if(c == -1){
            close(sockfd);
            perror("not connected");
            return -1;
        }
    // }

    freeaddrinfo(servinfo);

    if(send(sockfd, packet, BUFSIZE, 0) < 0){
        perror("sockfd send error");
        return -1;
    }
    memset(response, '\0', BUFSIZE);
    if((n=recv(sockfd, response, BUFSIZE, MSG_WAITALL)) < 0){
        perror("sockfd recv error");
        return -1;
    }

    close(sockfd);

    return n;

}

void read_conf(char* conf_folders, char* conf_ips, char* conf_ports){
    char *filename=strcat(getenv("HOME"),"/dfc.conf");
    FILE* conf=fopen(filename, "r");
    if(conf == NULL){
        perror("conf not opened");
    }
    else{
        int reading=1;
        int i=0;
        while(reading && i<4){
            char server_word[7];
            char folder[5];
            char address[24];
            char *ip=NULL;
            char *port=NULL;
            int fs=fscanf(conf, "%s %s %s", server_word, folder, address);
            if(fs != 3){
                reading = 0;
            }
            else{
                //delimeting each by a space
                char* delim=":";
                ip=strtok(address, delim);
                port=strtok(NULL, delim);
                strcat(conf_folders, folder);
                strcat(conf_folders, " ");
                strcat(conf_ips, ip);
                strcat(conf_ips, " ");
                strcat(conf_ports, port);
                strcat(conf_ports, " ");
            }
        }
        fclose(conf);
    }
}

void client_put(char* filename){
    FILE* readFile=fopen(filename, "rb");
    if(readFile == NULL){
        printf("error: %s not able to be opened", filename);
    }
    else{
        //get length of file in bytes
        long int length_of_file;
        fseek(readFile, 0, SEEK_END);
        length_of_file=ftell(readFile);
        fseek(readFile, 0, SEEK_SET);

        //get servers
        char* conf_folders=(char*)calloc(24, sizeof(char));
        memset(conf_folders,'\0',24);
        char* conf_ips=(char*)calloc(100, sizeof(char));
        memset(conf_ips, '\0', 100);
        char* conf_ports=(char*)calloc(100, sizeof(char));
        memset(conf_ports, '\0', 100);
        read_conf(conf_folders, conf_ips, conf_ports);
        char* save_folder;
        char* save_ip;
        char* save_port;
        char *folder=strtok_r(conf_folders, " ", save_folder);
        char* ip=strtok_r(conf_ips, " ", save_ip);
        char* port=strtok_r(conf_ports, " ", save_port);


        int left_over_from_split = length_of_file % 4;
        int split_bytes = (length_of_file - left_over_from_split)/4;
        //construct and send packets
        int sending = 1;
        int i=0;
        int x=hash(filename) % 4;

        char* part1=(char*)calloc(split_bytes+left_over_from_split, sizeof(char));
        char* part2=(char*)calloc(split_bytes, sizeof(char));
        char* part3=(char*)calloc(split_bytes, sizeof(char));
        char* part4=(char*)calloc(split_bytes, sizeof(char));
        memset(part1, '\0', split_bytes+left_over_from_split);
        memset(part2, '\0', split_bytes);
        memset(part3, '\0', split_bytes);
        memset(part4, '\0', split_bytes);

        if(!fread(part1, split_bytes+left_over_from_split, 1, readFile) == split_bytes+left_over_from_split){
            printf("error reading in part1 of file");
        }
        if(!fread(part2, split_bytes, 1, readFile) == split_bytes){
            printf("error reading in part2 of file");
        }
        if(!fread(part3, split_bytes, 1, readFile) == split_bytes){
            printf("error reading in part3 of file");
        }
        if(!fread(part4, split_bytes, 1, readFile) == split_bytes){
            printf("error reading in part4 of file");
        }
        // printf("%s %s %s %s", part1, part2, part3, part4);

        // while(sending){
        //     int p1=x+1;
        //     int p2;
        //     if(p1==4){
        //         p2=1;
        //     }
        //     else{
        //         p2=p1+1;
        //     }

        //     int bytes_left = split_bytes;
        //     //add left on the first split
        //     if(i==0){
        //         bytes_left = bytes_left + left_over_from_split;
        //     }

        // }
    }

}


int main(int argc, char **argv){

    // char* conf_folders=(char*)calloc(24, sizeof(char));
    // memset(conf_folders,'\0',24);
    // char* conf_ips=(char*)calloc(100, sizeof(char));
    // memset(conf_ips, '\0', 100);
    // char* conf_ports=(char*)calloc(100, sizeof(char));
    // memset(conf_ports, '\0', 100);
    // read_conf(conf_folders, conf_ips, conf_ports);
    // printf("%s %s %s", conf_folders, conf_ips, conf_ports);

    client_put(argv[1]);


    // if(argc == 1){
    //     printf("error: no command");
    // }
    // else if(strcmp(argv[1], "List") == 0){
    //     //do list
    // }
    // else if(strcmp(argv[1], "Get") == 0){
    //     //do get
    // }
    // else if(strcmp(argv[1], "Put") == 0){
    //     //do put
    // }
    // else{
    //     printf("error: incorrect command");
    // }

    // char *packet="hi";
    // char* response=(char*)calloc(BUFSIZE, sizeof(char));
    
    // char* folder="dfs1";
    // char* ip="198.59.7.14";
    // char* port="6587";
    // int size=BUFSIZE;
    // send_packet(packet, response, folder, ip, port, size);


    return 0;
}