
//we have to handle two fds only- socket fd and fd from terminal

#include<stdio.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<ctype.h>
#define SERV_TCP_PORT 5035      //port server
#define MAXLINE 4096            //max buffer size 

int strcicmp(char const *a, char const *b) //doubt?
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
    //return 0
}

void str_cli(FILE *fp, int sockfd)
{
    //fp: stdin, sockfd-client
    int         maxfdp1, stdineof;
    fd_set      rset;
    char        buf[MAXLINE],recv[MAXLINE];
    int     	n;

    stdineof = 0;   //sets 1 if end of file reached
    FD_ZERO(&rset); //init fdset to zero : required

    //runs infintely
    for ( ; ; ) {

        //2 bufferes created of size MAXLINE and init to zero
    	bzero(recv,sizeof(recv));
    	bzero(buf,sizeof(buf));
    	
        //when initially connected this is set to 0 and we are adding our file des(stdin-0) - terminal 
        //file pointer from where this will recieve data for reading (set to readable set)
        if (stdineof == 0)
            FD_SET(fileno(fp), &rset); //1: file des, fd readable set
        
        FD_SET(sockfd, &rset);  //required
        maxfdp1 = (sockfd > 0 ? sockfd : 0) + 1;
        //maxfd1=max(0,sockfd)+1

        select(maxfdp1, &rset, NULL, NULL, NULL);  //calling select

        if (FD_ISSET(sockfd, &rset)) {  /* socket is readable and from server to terminal*/
            if ( (n = read(sockfd, recv, MAXLINE)) == 0) {
                if (stdineof == 1)
                    return;     /* normal termination FIN recieved*/
                else{
                    printf("\nstr_cli: server terminated prematurely\n"); 
					return;
				}
            }
			recv[n]='\0';
			write(1,recv,strlen(recv));
        }

        if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable from terminal */
            //read from stdin/terminal to buf
            if ( (n = read(fileno(fp), buf, MAXLINE)) == 0 || stdineof) {  
                stdineof = 1;   //ctrl c recv so stdeof set and shutdown 
                shutdown(sockfd, SHUT_WR);  /* send FIN */
                FD_CLR(fileno(fp), &rset);  //cannot set terminal because no more data so clear this from fd_set
                continue;
            }

            //close client connection
			if(strcicmp(buf,"end\n")==0 || strcicmp(buf,"bye\n")==0)
                stdineof = 1;			
			
            write(sockfd, buf, strlen(buf));
        }
    }
}

int main(){
	
	int sockFd = socket(AF_INET, SOCK_STREAM, 0); 
	struct sockaddr_in serv_addr;   //server socket address structure IPV4
	

	sockFd = socket(AF_INET,SOCK_STREAM,0); //1. socket family

    serv_addr.sin_family = AF_INET;         //server family
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //Ip address ascii to network byte order
    serv_addr.sin_port = htons(SERV_TCP_PORT);  //host byte order to netwrk byte order, defined above port
    //port should not be any well-defined, reserved

    //write(1,"\nEnter username:",16);        
    //1: std out(terminal) write
    //buff : write to terminal
    //16: size of buff
	
    //connect return if error
	if(connect(sockFd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
		printf("Error");
		return 1;
	}

	str_cli(stdin,sockFd);  //functionality of client after connection to be performed
	
	return 0;
}
