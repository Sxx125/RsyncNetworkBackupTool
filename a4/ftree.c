#include <stdio.h>
#include <stdlib.h>
#include "ftree.h"
#include "hash.h"
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#define BUF_SIZE 256
#define MAX_BACKLOG 5
#define MAX_CONNECTIONS 12

#ifndef PORT
  #define PORT 51888
#endif

struct sockname {
    int sock_fd;
    //char *username;
    int client_type;
};

int children = 0;
//char arr*;
//struct fileinfo *fi;

int check_hash(const char *hash1, const char *hash2) {
    for (int i = 0; i < BLOCKSIZE; i++){
        if (hash1[i]!=hash2[i]) {
            return MISMATCH;
        }
    }

    return MATCH;


}

void wait_for(int sock, char *path, struct fileinfo fi){
    int m;

    read(sock,&m,sizeof(int));
    int match = ntohl(m);
    if(match==MATCH){
        //do nothing

    }
    if(match==MISMATCH){

        pid_t result = fork();

        children++;
        if(result==0){
            //connect as sender
            int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd < 0) {
                perror("client: socket");
                exit(1);
            }

            // Set the IP and port of the server to connect to.
            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(PORT);
            if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
                perror("client: inet_pton");
                close(sock_fd);
                exit(1);
            }

            // Connect to the server.
            if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
                perror("client: connect");
                close(sock_fd);
                exit(1);
            }



            int sender_client = htonl(SENDER_CLIENT);

            write(sock_fd,&sender_client,sizeof(int));

            write(sock_fd, fi.path, MAXPATH);
            mode_t modet = htonl(fi.mode);
            write(sock_fd, &modet,sizeof(mode_t));
            //FILE *f = fopen(str,"rb");
            write(sock_fd,fi.hash,BLOCKSIZE);
            size_t sizet = htonl(fi.size);
            write(sock_fd,&sizet,sizeof(size_t));

            struct stat st;
            if(lstat(path,&st) == -1){
                //printf("%s\n",path);
                //fflush(stdout);
                perror("lstat");
                exit(1);
            }

            if(S_ISDIR(st.st_mode)){
                //CRY
            } else {
                char buf[MAXDATA];
                FILE *f = fopen(path,"rb");
                while(fread(buf,sizeof(buf),MAXDATA,f)){
                    write(sock,buf,MAXDATA);
                }
                fclose(f);
                //printf("Done sending file info\n");
                //fflush(stdout);
                //server chmod yep
                //recieve TRANSMIT_OK yep
                int transmit;
                read(sock_fd,&transmit,sizeof(int));
                int trans = ntohl(transmit);
                if(trans == TRANSMIT_OK){
                    close(sock_fd);
                    exit(0);
                    //printf("Child died\n");
                    //fflush(stdout);
                } else {
                    exit(1);
                }
                
            }
            

        }
    }
    if(match==MATCH_ERROR){

        close(sock);
        exit(1);
    }
    
}

int path_walker(char *path, int sock_fd, char *dest_path, char* append){
    
    struct fileinfo fi;
    char nll[BLOCKSIZE];
    for(int i=0; i < BLOCKSIZE;i++){
        nll[i]='0';
    }
    struct stat lst;
    DIR *d = opendir(path);
    struct dirent *sd;
    while((sd=readdir(d))){
        if(sd->d_name[0]=='.'){
            continue;
        } else {
            char *str = malloc(strlen(path)+strlen(sd->d_name)+2);
            strcpy(str,path);
            strcat(str,"/");
            strcat(str,sd->d_name);
            if(lstat(str,&lst) == -1){
                perror("lstat");
                exit(1);
            }
            if(S_ISDIR(lst.st_mode)){

                char* dest = malloc(strlen(basename(str))+strlen(append)+strlen(dest_path)+3);
                strcpy(dest,dest_path);
                strcat(dest,"/");
                strcat(dest,append);
                strcat(dest,"/");
                strcat(dest,basename(str));
                //printf("%s",dest);
                //fflush(stdout);

                write(sock_fd, dest, MAXPATH);
                mode_t modet = htonl(lst.st_mode);
                write(sock_fd, &modet,sizeof(mode_t));
                write(sock_fd,nll,BLOCKSIZE);
                size_t sizet = htonl(lst.st_size);
                write(sock_fd,&sizet,sizeof(size_t));
                
                strcpy(fi.path,dest);
                fi.mode= lst.st_mode;
                //strcpy(fi.hash,nll); 
				for(int j =0; j<BLOCKSIZE;j++){
					fi.hash[j]=nll[j];
				}
                fi.size = lst.st_size;
                wait_for(sock_fd,str,fi);

                char* append2 = malloc(strlen(append)+strlen(basename(str))+2);
                strcpy(append2,append);
                strcat(append2,"/");
                strcat(append2,basename(str));
                path_walker(str,sock_fd,dest_path,append2);
                free(str);
            } else {

                char* dest = malloc(strlen(basename(str))+strlen(append)+strlen(dest_path)+3);
                strcpy(dest,dest_path);
                strcat(dest,"/");
                strcat(dest,append);
                strcat(dest,"/");
                strcat(dest,basename(str));
                //printf("%s",dest);
                //fflush(stdout);

                write(sock_fd, dest, MAXPATH);
                mode_t modet = htonl(lst.st_mode);
                write(sock_fd, &modet,sizeof(mode_t));
                FILE *f = fopen(str,"rb");
                write(sock_fd,hash(f),BLOCKSIZE);
                fclose(f);
                f = fopen(str,"rb");
                size_t sizet = htonl(lst.st_size);
                write(sock_fd,&sizet,sizeof(size_t));
                
                strcpy(fi.path,dest);
                fi.mode = lst.st_mode;
                //strcpy(fi.hash,hash(f));
				char* arr = hash(f);
				for(int j =0; j<BLOCKSIZE;j++){
					fi.hash[j]=arr[j];
				}
                fi.size = lst.st_size;
                wait_for(sock_fd,str,fi);
                fclose(f);

            }
                

        }

    }
    return 0;
}

int rcopy_client(char *src_path, char *dest_path, char *host_ip, int port) {
     printf("SRC: %s\n", src_path);
     printf("DEST: %s\n", dest_path);
     printf("IP: %s\n", host_ip);
     printf("PORT: %d\n", port);
     fflush(stdout);


    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        exit(1);
    }

    int num = htonl(CHECKER_CLIENT);
    write(sock_fd, &num, sizeof(int));




    struct stat st;
    if(lstat(src_path,&st) == -1){
        perror("lstat");
        exit(1);
    }

    char *src = malloc(sizeof(char)*strlen(src_path));
    strcpy(src,src_path);

    char nll[BLOCKSIZE];
    for(int i=0; i < BLOCKSIZE;i++){
        nll[i]='0';
    }
    struct fileinfo fi;
    if(S_ISDIR(st.st_mode)){
        char* append = basename(src);
        char* dest = malloc(strlen(basename(src))+strlen(dest_path)+2);
        strcpy(dest,dest_path);
        strcat(dest,"/");
        strcat(dest,src);
        //printf("%s",dest);
        //fflush(stdout);

        write(sock_fd, dest, MAXPATH);
        mode_t modet = htonl(st.st_mode);
        write(sock_fd, &modet,sizeof(mode_t));
        write(sock_fd,nll,BLOCKSIZE);
        size_t sizet = htonl(st.st_size);
        write(sock_fd,&sizet,sizeof(size_t));

        strcpy(fi.path,dest);
        fi.mode= st.st_mode;
        //strcpy(fi.hash,nll);
		for(int j =0; j<BLOCKSIZE;j++){
			fi.hash[j]=nll[j];
		}
        fi.size = st.st_size;

        wait_for(sock_fd,src,fi);
        path_walker(src,sock_fd,dest_path,append);
    } else {
        char* dest = malloc(strlen(basename(src))+strlen(dest_path)+2);
        strcpy(dest,dest_path);
        strcat(dest,"/");
        strcat(dest,basename(src));
        //printf("%s",dest);
        //fflush(stdout);

        write(sock_fd, dest, MAXPATH);
        mode_t modet = htonl(st.st_mode);
        write(sock_fd, &modet,sizeof(mode_t));
        FILE *f = fopen(src,"rb");
        write(sock_fd,hash(f),BLOCKSIZE);
        fclose(f);
        f = fopen(src,"rb");
        size_t sizet = htonl(st.st_size);
        write(sock_fd,&sizet,sizeof(size_t));

        strcpy(fi.path,dest);
        fi.mode= st.st_mode;
        //strcpy(fi.hash,hash(f));
		char* arr = hash(f);
		for(int j =0; j<BLOCKSIZE;j++){
			fi.hash[j]=arr[j];
		}
        fi.size = st.st_size;
        wait_for(sock_fd,src,fi);
        fclose(f);
    }

    for(int i = 0; i < children; i++){
        int status;
        pid_t pid;
        if((pid = wait(&status)) == -1 ){
            close(sock_fd);
            exit(1);
        }
    }
    close(sock_fd);

    return 0;
}



int accept_connection(int fd, struct sockname *client_list) {

    int user_index = 0;
    
    int num;
    while (user_index < MAX_CONNECTIONS && client_list[user_index].sock_fd != -1) {
        user_index++;
    }

    int client_fd = accept(fd, NULL, NULL);

    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    if (user_index == MAX_CONNECTIONS) {
        fprintf(stderr, "server: max concurrent connections\n");
        close(client_fd);
        return -1;
    }
    //read(client_fd,buf,sizeof(buf));
    //char buf[256];
    //int type;

    read(client_fd,&num,sizeof(int));
    int n = ntohl(num);

    client_list[user_index].client_type = n;
    client_list[user_index].sock_fd = client_fd;

    struct fileinfo fi;
    struct stat server_stat;

    if(client_list[user_index].client_type==CHECKER_CLIENT){
        //TODO
        int match = htonl(MATCH);
        int mismatch = htonl(MISMATCH);
        int match_error = htonl(MATCH_ERROR);
        

            read(client_fd,fi.path,MAXPATH);

            mode_t modet;
            read(client_fd,&modet,sizeof(mode_t));
            fi.mode = ntohl(modet);

            read(client_fd,fi.hash,BLOCKSIZE);

            size_t sizet;
            read(client_fd,&sizet,sizeof(size_t));
            fi.size = ntohl(sizet);

            if(lstat(fi.path,&server_stat) == -1){// path doesnt exist
                /*
                perror("lstat");
                exit(1);
                */
                char* fakepath = malloc(strlen(fi.path)+1);
                strcpy(fakepath,fi.path);
                char *ppath = dirname(fakepath);
                if(lstat(ppath,&server_stat) == -1){ // parent path doesnt exist

                    write(client_fd,&match_error,sizeof(int));
                    //break;
                }
                if(S_ISDIR(fi.mode)){ // dir
                    mkdir(fi.path,fi.mode);
                    //chmod(fi.path,fi.mode);
                    //permissions
                    write(client_fd,&match,sizeof(int));
                } else { // file
                    FILE *s = fopen(fi.path,"wb");
                    write(client_fd,&mismatch,sizeof(int));
                    fclose(s);
                }
            } else { // path exists
                if(S_ISDIR(server_stat.st_mode) != S_ISDIR(fi.mode)){ // types dont match

                    write(client_fd,&match_error,sizeof(int));
                } else {
                    if(S_ISDIR(fi.mode)){//Dir
                        if(fi.mode==server_stat.st_mode){//permissions same
                            write(client_fd,&match,sizeof(int));
                        } else {
                            chmod(fi.path,fi.mode);
                            write(client_fd,&match,sizeof(int));
                        }
                    } else { //file
                        if(fi.size!=server_stat.st_size){//sizes dont match
                            write(client_fd,&mismatch,sizeof(int));
                        } else {
                            FILE *f = fopen(fi.path,"rb");
                            if(check_hash(fi.hash,hash(f))==MATCH){// hashes match
                                chmod(fi.path,fi.mode);
                                write(client_fd,&match,sizeof(int));
                            } else {
                                write(client_fd,&mismatch,sizeof(int));
                            }
                            fclose(f);
                        }
                    }
                }
            }

            

            

        return client_fd;
        
    }

    if(client_list[user_index].client_type==SENDER_CLIENT){

        
        read(client_fd,fi.path,MAXPATH);
        mode_t modet;
        read(client_fd,&modet,sizeof(mode_t));
        fi.mode = ntohl(modet);
        read(client_fd,fi.hash,BLOCKSIZE);
        size_t sizet;
        read(client_fd,&sizet,sizeof(size_t));
        fi.size = ntohl(sizet);
        if(lstat(fi.path,&server_stat) == -1){
            perror("lstat");
            exit(1);
        }


        fflush(stdout);

        if(S_ISDIR(server_stat.st_mode)){
            //???
        } else {
            FILE *f = fopen(fi.path,"a");
            //permissions
            int numread = 0;
            char* buf[MAXDATA];
            
            printf("reading file...");
            fflush(stdout);
            while(numread != fi.size){
                
                numread += read(client_fd,buf,MAXDATA);
                fwrite(buf,sizeof(char),numread,f);
            }
            fclose(f);
            chmod(fi.path,fi.mode);
            int transmit_ok = htonl(TRANSMIT_OK);
            write(client_fd,&transmit_ok,sizeof(int));
        }
        
    } else {
        perror("Type:");
        close(client_fd);
        exit(1);
    }

    return client_fd;
}

void rcopy_server(int port) {
    printf("PORT: %d\n", port);
    struct sockname client_list[MAX_CONNECTIONS];
    for (int index = 0; index < MAX_CONNECTIONS; index++) {
        //printf("in for loop");
        client_list[index].sock_fd = -1;
        //client_list[index].client_type = NULL;
    }

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    memset(&server.sin_zero, 0, 8);

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }


    int max_fd = sock_fd;
    fd_set all_fds, listen_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

    while(1){
        // select updates the fd_set it receives, so we always use a copy and retain the original.
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, client_list);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);

        }

        for (int index = 0; index < MAX_CONNECTIONS; index++) {
            if (client_list[index].sock_fd > -1 && FD_ISSET(client_list[index].sock_fd, &listen_fds)) {
                int client_fd = client_list[index].sock_fd;
                struct fileinfo fi;
                struct stat server_stat;

                if(client_list[index].client_type==CHECKER_CLIENT){
                    //TODO
                    int match = htonl(MATCH);
                    int mismatch = htonl(MISMATCH);
                    int match_error = htonl(MATCH_ERROR);
                    

                        read(client_fd,fi.path,MAXPATH);

                        mode_t modet;
                        read(client_fd,&modet,sizeof(mode_t));
                        fi.mode = ntohl(modet);

                        read(client_fd,fi.hash,BLOCKSIZE);

                        size_t sizet;
                        read(client_fd,&sizet,sizeof(size_t));
                        fi.size = ntohl(sizet);

                        if(lstat(fi.path,&server_stat) == -1){// path doesnt exist
                            /*
                            perror("lstat");
                            exit(1);
                            */
                            char* fakepath = malloc(strlen(fi.path)+1);
                            strcpy(fakepath,fi.path);
                            char *ppath = dirname(fakepath);
                            if(lstat(ppath,&server_stat) == -1){ // parent path doesnt exist

                                write(client_fd,&match_error,sizeof(int));
                                //break;
                            }
                            if(S_ISDIR(fi.mode)){ // dir
                                mkdir(fi.path,fi.mode);
                                //chmod(fi.path,fi.mode);
                                //permissions
                                write(client_fd,&match,sizeof(int));
                            } else { // file
                                FILE *s = fopen(fi.path,"wb");
                                write(client_fd,&mismatch,sizeof(int));
                                fclose(s);
                            }
                        } else { // path exists
                            if(S_ISDIR(server_stat.st_mode) != S_ISDIR(fi.mode)){ // types dont match
                                
                                write(client_fd,&match_error,sizeof(int));
                            } else {
                                if(S_ISDIR(fi.mode)){//Dir
                                    if(fi.mode==server_stat.st_mode){//permissions same
                                        write(client_fd,&match,sizeof(int));
                                    } else {
                                        chmod(fi.path,fi.mode);
                                        write(client_fd,&match,sizeof(int));
                                    }
                                } else { //file
                                    if(fi.size!=server_stat.st_size){//sizes dont match
                                        write(client_fd,&mismatch,sizeof(int));
                                    } else {
                                        FILE *f = fopen(fi.path,"rb");
                                        if(check_hash(fi.hash,hash(f))==MATCH){// hashes match
                                            chmod(fi.path,fi.mode);
                                            write(client_fd,&match,sizeof(int));
                                        } else {
                                            write(client_fd,&mismatch,sizeof(int));
                                        }
                                        fclose(f);
                                    }
                                }
                            }
                        }

                        
                    
                }
            }
        }

    }

}


