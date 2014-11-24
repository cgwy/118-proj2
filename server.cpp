#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet.h"
#include "controller.h"

#define BUFSIZE 2048

/*
 * error - wrapper for perror
 */
void error(string msg) {
    perror(msg.c_str());
    exit(1);
}

void checkArgv(int argc, char **argv)
{
	congestion_control_on = false;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port> <enable_congestion_control>\n", argv[0]);
        exit(1);
    }
	else if(argc >= 3)
	{
		if(strcmp(argv[2], "1")==0)
			congestion_control_on = true;
	}
}

int main(int argc, char **argv) {
	checkArgv(argc, argv);

    int sockfd; /* socket */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char buf[BUFSIZE]; /* message buf */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */
    
    int portno = atoi(argv[1]);
    
    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval , sizeof(int));
    
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
    if (bind(sockfd, (struct sockaddr *) &serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");
    
    /*
     * main loop: wait for a datagram, then echo it
     */
    clientlen = sizeof(clientaddr);
    while (1) {
        bzero(buf, BUFSIZE);
        n = recvfrom(sockfd, buf, BUFSIZE, 0,
                     (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
        if (n < 0)
            error("ERROR in recvfrom");
        
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                              sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");
        printf("server received datagram from %s (%s)\n",
               hostp->h_name, hostaddrp);
        printf("server received %lu/%d bytes: %s\n", strlen(buf), n, buf);
        
		static Controller ctrls;

		string host = string(hostaddrp); 
		vector<string> respVec = ctrls.handleNewPkt(host, string(buf));
		for(int i=0;i<respVec.size();i++)
		{
			string resp = respVec[i];
			n = sendto(sockfd, resp.c_str(), resp.length(), 0, 
                   (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
				error("ERROR in sendto");
			else
				fprintf(stderr, "server.cpp-120: sent %s\n", resp.c_str());
		}
    }
}
