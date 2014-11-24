/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <cstdio>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>
#include <fstream>
#include <ctime>
using namespace std;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
	 fprintf(stderr,"ERROR, no port provided\n");
	 exit(1);
	}
	int portno = atoi(argv[1]);

	//struct sigaction sa;          // for signal SIGCHLD

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) 
		error("ERROR opening socket");
	
	struct sockaddr_in serv_addr, client_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sock, (struct sockaddr *) &serv_addr,
		  sizeof(serv_addr)) < 0) 
		  error("ERROR on binding");

	if(listen(sock,5) == -1)
		error("server-listen error");

	socklen_t clilen = sizeof(client_addr);
	fd_set active_fd_set;

	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);
	int new_sock;
	while(1)
	{
		if( select(sock+1, &active_fd_set, NULL, NULL, NULL)<0 )
				error("select error");
		else
			puts("select done");
		
		if(FD_ISSET(sock, &active_fd_set))
		{
			new_sock = accept(sock, (struct sockaddr *)&client_addr, &clilen);
			FD_SET(new_sock, &active_fd_set);
		}
		if(FD_ISSET(new_sock, &active_fd_set))
		{
			dostuff(new_sock);
		}
	}
	
	return 0; /* we never get here */	
}

void dostuff (int sock)
{
	int n;
	const int MAX_LEN = 256;
	char buffer[MAX_LEN];
      
	bzero(buffer,MAX_LEN);
	n = read(sock,buffer,MAX_LEN-1);
	if (n < 0) 
		error("ERROR reading from socket");
	printf("Here is the message: %s\n",buffer);

	char resp[] = "server ok";
	n = write(sock, resp, strlen(resp));
   
	if (n < 0) 
		error("ERROR writing to socket");
}
