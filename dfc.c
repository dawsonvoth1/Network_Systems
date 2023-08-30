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

#include <openssl/md5.h>
#include <sys/time.h>

#define MAX_ARG_SIZE 1000
#define BUFSIZE 600000
#define CHUNKSIZE 250000

int file_chunk_table[4][4][2]={
  {{ 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 1 }},
  {{ 4, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }},
  {{ 3, 4 }, { 4, 1 }, { 1, 2 }, { 2, 3 }},
  {{ 2, 3 }, { 3, 4 }, { 4, 1 }, { 1, 2 }}
};

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

    if(send(sockfd, packet, size, 0) < 0){
        perror("sockfd send error");
        return -1;
    }
    memset(response, '\0', BUFSIZE);
    if((n=recv(sockfd, response, BUFSIZE, MSG_WAITALL)) < 0){
        perror("sockfd recv error");
        return -1;
    }
    FILE* t=fopen("prac.txt", "w+");
        fwrite(response, 150, 1, t);
        fclose(t);
    // printf("%xd\n", response);

    close(sockfd);

    return n;

}

void read_conf(char conf_folders[4][10], char conf_ips[4][20], char conf_ports[4][10]){
    char filename[100];
    strcat(filename, getenv("HOME"));
    strcat(filename, "/dfc.conf");
    // char *filename=strcat(getenv("HOME"),"/dfc.conf");
    FILE* conf=fopen(filename, "r");
    if(conf == NULL){
        printf("%s\n", filename);
        perror("conf not opened");
    }
    else{
        int reading=1;
        int i=0;
        while(reading && i<4){
            char server_word[7];
            char folder[10];
            char address[30];
            char *ip=NULL;
            char *port=NULL;
            int fs=fscanf(conf, "%s %s %s", server_word, folder, address);
            if(fs != 3){
                reading = 0;
            }
            else{
                char* delim=":";
                ip=strtok(address, delim);
                port=strtok(NULL, delim);
                strcpy(conf_folders[i], folder);
                strcpy(conf_ips[i], ip);
                strcpy(conf_ports[i], port);
            }
            i++;
        }
        fclose(conf);
    }
}

// Put
// <filename>
// <timeval>
// <chunk #>
// <chunk 1 data>
// <chunk #>
// <chunk 2 data>

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
        char conf_folders[4][10], conf_ips[4][20], conf_ports[4][10];
        read_conf(conf_folders, conf_ips, conf_ports);

        //construct and send packets
        int left_over_from_split = length_of_file % 4;
        int split_bytes = (length_of_file - left_over_from_split)/4;
        printf("%d : %d\n", left_over_from_split, split_bytes);

        struct timeval curr_t;
        gettimeofday(&curr_t, NULL);

        printf("%ld, %ld,", curr_t.tv_sec, curr_t.tv_usec);

        //md5 hash filename
        unsigned long n=strlen(filename);
        unsigned char md[MD5_DIGEST_LENGTH];
        unsigned char* res=MD5(filename, n, md);
        int x = md[MD5_DIGEST_LENGTH - 1] % 4;
        // printf("%d\n", x);

        char* method="Put";
        char* delim="\r\n\r\n";
        char header[200]; // =(char*)calloc(200, sizeof(char));
        memset(header, '\0', 200);
        strcat(header, method);
        strcat(header, delim);
        strcat(header, filename);
        strcat(header, delim);
        int location_in_buf=strlen(header);
        memcpy(header+location_in_buf, &curr_t, sizeof(curr_t));
        location_in_buf = location_in_buf + sizeof(curr_t);
        memcpy(header+location_in_buf, delim, strlen(delim));
        location_in_buf = location_in_buf + strlen(delim);

        int length_of_head=location_in_buf;


        // FILE* t=fopen("prac.txt", "w+");
        // fwrite(header, 150, 1, t);
        // fclose(t);

        int sending = 1;
        int i=0;

        char* chunk1=(char*)calloc(split_bytes+left_over_from_split, sizeof(char));
        char* chunk2=(char*)calloc(split_bytes+left_over_from_split, sizeof(char));
        int p1=x;
        int p2;
        while(sending != 0 && i < 4){
            p1++;
            if(p1==4){
                p2=1;
            }
            else if(p1 > 4){
                p1=1;
                p2=2;
            }
            else{
                p2=p1+1;
            }
            location_in_buf=length_of_head;
            //add chunk #s and chunk data to packet
            char buf[BUFSIZE];
            memset(buf, '\0', BUFSIZE);
            memcpy(buf, header, location_in_buf);
            
            char n1[2];
            sprintf(n1, "%d", p1);
            memcpy(buf+location_in_buf, n1, sizeof(n1));
            location_in_buf = location_in_buf + sizeof(n1);

            memcpy(buf+location_in_buf, delim, strlen(delim));
            location_in_buf = location_in_buf + strlen(delim);

            fseek(readFile, 0, SEEK_SET);

            memset(chunk1, '\0', split_bytes+left_over_from_split);
            if(p1 == 1){
                if(fread(chunk1, split_bytes+left_over_from_split, 1, readFile) == split_bytes+left_over_from_split){
                    printf("error reading in chunk1 of file\n");
                }
                else{
                    memcpy(buf+location_in_buf, chunk1, split_bytes+left_over_from_split);
                    location_in_buf = location_in_buf + split_bytes + left_over_from_split;
                }
            }
            else{
                int offset=split_bytes*(p1-1) + left_over_from_split;
                fseek(readFile, offset, SEEK_SET);
                if(fread(chunk1, split_bytes, 1, readFile) == split_bytes){
                    printf("error reading in chunk1 of file\n");
                }
                else{
                    memcpy(buf+location_in_buf, chunk1, split_bytes);
                    location_in_buf = location_in_buf + split_bytes;
                }
            }
            memcpy(buf+location_in_buf, delim, strlen(delim));
            location_in_buf = location_in_buf + strlen(delim);

            char n2[2];
            sprintf(n2, "%d", p2);
            memcpy(buf+location_in_buf, n2, sizeof(n2));
            location_in_buf = location_in_buf + sizeof(n2);

            memcpy(buf+location_in_buf, delim, strlen(delim));
            location_in_buf = location_in_buf + strlen(delim);

            memset(chunk2, '\0', split_bytes+left_over_from_split);
            if(p2 == 1){
                fseek(readFile, 0, SEEK_SET);
                if(fread(chunk2, split_bytes+left_over_from_split, 1, readFile) == split_bytes+left_over_from_split){
                    printf("error reading in chunk2 of file\n");
                }
                else{
                    memcpy(buf+location_in_buf, chunk2, split_bytes+left_over_from_split);
                    location_in_buf = location_in_buf + split_bytes + left_over_from_split;
                }
            }
            else{
                if(fread(chunk2, split_bytes, 1, readFile) == split_bytes){
                    printf("error reading in chunk2 of file\n");
                }
                else{
                    memcpy(buf+location_in_buf, chunk2, split_bytes);
                    location_in_buf = location_in_buf + split_bytes;
                }
            }
            memcpy(buf+location_in_buf, delim, strlen(delim));
            location_in_buf = location_in_buf + strlen(delim);

            // FILE* t=fopen("prac.txt", "w+");
            // fwrite(buf, location_in_buf, 1, t);
            // fclose(t);

            char response[BUFSIZE];
            memset(response, '\0', BUFSIZE);

            if(send_packet(buf, response, conf_folders[i], conf_ips[i], conf_ports[i], location_in_buf) < 0 || strcmp(response, "failed") == 0){
                sending =0;
                // break;
            }


            // printf("%s\n", conf_folders[0]);
            

            // sending=0;
            i++;
        }
        if(sending == 0){
            char response[BUFSIZE];
            memset(response, '\0', BUFSIZE);
            for(int j=0; j<4; j++){
                char delete_command[150];
                memset(delete_command, '\0', 150);
                strcat(delete_command, "Delete\r\n\r\n");
                strcat(delete_command, filename);
                memset(response, '\0', BUFSIZE);
                send_packet(delete_command, response, conf_folders[j], conf_ips[j], conf_ports[j], strlen(delete_command));
                printf("%s\n", response);
            }
            printf("%s put failed.\n", filename);
        }
        fclose(readFile);
    }
}


int client_get(char* filename){
    //get servers
    char conf_folders[4][10], conf_ips[4][20], conf_ports[4][10];
    read_conf(conf_folders, conf_ips, conf_ports);

    int sending = 1;
    int i=0;

    //md5 hash filename
    unsigned long n=strlen(filename);
    unsigned char md[MD5_DIGEST_LENGTH];
    unsigned char* res=MD5(filename, n, md);
    int x = md[MD5_DIGEST_LENGTH - 1] % 4;

    char chunks[4][CHUNKSIZE];//=(char*)calloc(CHUNKSIZE, sizeof(char));
    memset(chunks, '\0', CHUNKSIZE*4);
    
    int chunk_sizes[4]={0,0,0,0};
    
    // int *curr_p1=(int*)calloc(1, sizeof(int));
    // int *curr_p2=(int*)calloc(1, sizeof(int));
    int p1=x;
    int p2;
    int servers_down=0;
    while(sending != 0 && i < 4){
        char curr_p1[2];
        char curr_p2[2];
        memset(curr_p1, '\0', 2);
        memset(curr_p2, '\0', 2);
        p1++;
        if(p1==4){
            p2=1;
        }
        else if(p1 > 4){
            p1=1;
            p2=2;
        }
        else{
            p2=p1+1;
        }
        char response[BUFSIZE];
        memset(response, '\0', BUFSIZE);
        char get_command[150];
        memset(get_command, '\0', 150);
        strcat(get_command, "Get\r\n\r\n");
        strcat(get_command, filename);
        char *res_filename;//[100];
        struct timeval res_t;
        // memset(res_filename, '\0', 100);
        char* delim="\r\n\r\n";
        int n=0;
        int location_in_res=0;
        if((n=send_packet(get_command, response, conf_folders[i], conf_ips[i], conf_ports[i], strlen(get_command))) < 0){
            printf("error get failed on server %s\n", conf_folders[i]);
            servers_down++;
        }
        else{
            res_filename=strtok(response, delim);
            printf("%s\n", res_filename);
            location_in_res=location_in_res + strlen(res_filename) + strlen(delim);
            memcpy(&res_t, response + location_in_res, sizeof(res_t));
            location_in_res=location_in_res + sizeof(res_t)+ strlen(delim);
            // printf("%ld, %ld,\n", res_t.tv_sec, res_t.tv_usec);
            // printf("%d\n", location_in_res);
            memcpy(curr_p1, response + location_in_res, 2);
            location_in_res=location_in_res + 2 + strlen(delim);
            // printf("%s\n", response+location_in_res);
            if(p1 != strtol(curr_p1, NULL, 10)){
                printf("p1=1: %d %ld\n", p1, strtol(curr_p1, NULL, 10));
            }
            // printf("%d\n", location_in_res);
            char* end_of_p1 = memchr(response + location_in_res, '\r', CHUNKSIZE);
            long size_of_p1=end_of_p1 - response - location_in_res; 
            // printf("%p %p\n", end_of_p1, response);
            // printf("p1 size: %d\n", size_of_p1);
            chunk_sizes[p1-1]=size_of_p1;
            memcpy(chunks[p1-1], response + location_in_res, size_of_p1);
            location_in_res = location_in_res + size_of_p1 + strlen(delim);

            memcpy(curr_p2, response + location_in_res, 2);
            location_in_res=location_in_res + 2+ strlen(delim);
            if(p2 != strtol(curr_p2, NULL, 10)){
                printf("p2: %d %ld\n", p2, strtol(curr_p2, NULL, 10));
            }

            char* end_of_p2= (char*) memchr(response + location_in_res, '\r', CHUNKSIZE);
            long size_of_p2=end_of_p2 - response - location_in_res;
            chunk_sizes[p2-1]=size_of_p2;
            memcpy(chunks[p2-1], response + location_in_res, size_of_p2);
            location_in_res = location_in_res + size_of_p2 + strlen(delim);
        }

        i++;
    }
    if(servers_down > 1){
        printf("%s is incomplete\n", filename);
    }
    else{
        char temp_list_location[30];
        memset(temp_list_location, '\0', 30);
        strcat(temp_list_location, filename);
        // strcat(temp_list_location, "2");
        FILE* t=fopen(temp_list_location, "wb+");
        for(int j=0; j<4; j++){
            fwrite(chunks[j], 1, chunk_sizes[j], t);
        }
    }
    return 0;
}


int main(int argc, char **argv){


    if(argc == 1){
        printf("error: no commands format: ./dfc <command> [filenames]...\n");
    }
    else if(strcmp(argv[1], "List") == 0){
        //do list
    }
    else if(strcmp(argv[1], "Get") == 0){
        //do get
        for(int i=2; i<argc; i++){
            client_get(argv[i]);
        }
    }
    else if(strcmp(argv[1], "Put") == 0){
        //do put
        for(int i=2; i<argc; i++){
            client_put(argv[i]);
        }
    }
    else{
        printf("error: incorrect command");
    }


    return 0;
}