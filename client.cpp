/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "packet.h"

#define BUFSIZE 2048

bool congestion_control_on = true;
FILE *fp;

//print msg and exit
void error(const char *msg) 
{
	perror(msg);
	exit(0);
}

void checkArgv(int argc, char **argv)
{
	if(argc < 4)
	{
		fprintf(stderr, "usage:\n %s <send_hostname> <sender_portnumer> <filename> [<enable_congestion_control> = on/off]\t", argv[0]);
		exit(-1);
	}
	else if(argc >= 5)
	{
		if(strcmp(argv[4], "on")==0)
			congestion_control_on = true;
		else if(strcmp(argv[4], "off") == 0)
			congestion_control_on = false;
		else
			fprintf(stderr, "usage:\n %s <send_hostname> <sender_portnumer> <filename> [<enable_congestion_control> = on/off]\t", argv[0]);
	}
}

bool simulateCorruption()
{
	return !((rand()%10 + 1)%5);
}
bool simulateLoss()
{
	return !((rand()%10 + 1)%5);
}

//@param 
//recvBug: received string
//lastPkt: last sent packet.
FTPPacket parseRecv(string recvBuf, const FTPPacket& lastPkt)
{
	static int lastAck = -1;
	FTPPacket respPkt;
	FTPPacket recvPkt = FTPPacket::toPacket(recvBuf);
	if(recvPkt.isCorrupted() || simulateCorruption())
	{
		respPkt = lastPkt;
		respPkt.pktType = RECV_NAK;
		return lastPkt;
	}
	else
	{
		if(recvPkt.sequence > lastPkt.ack)// some earlier packet is lost
			return lastPkt;
		else if(recvPkt.sequence == lastPkt.ack)// right packet received
		{
			fwrite(recvPkt.payload.c_str(), 1, recvPkt.payload.length(), fp);
			if(recvPkt.payloadLen < MAX_PAYLOAD)// last Packet received!
			{
				return FTPPacket(0, 0, RECV_ACK, "done");
			}
			int newAck = recvPkt.payloadLen + recvPkt.sequence;
			return FTPPacket(0, newAck, RECV_ACK, "");
		}
		else//delayed old packet, ignore it
		{
			respPkt = lastPkt;
			respPkt.payload = "ignore";
		}
	}
	
	return respPkt;
}

int main(int argc, char **argv) {
	checkArgv(argc, argv);

    /* check command line arguments */
    char *hostname = argv[1];
    int portno = atoi(argv[2]);
	char *fName = argv[3];

    int sockfd, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char buf[BUFSIZE];
    
    // socket: create the socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    // gethostbyname: get the server's DNS entry
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }
    
    // build the server's Internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
	int serverlen = sizeof(serveraddr);
    
	FTPPacket lastPkt(0, 0, NEW_REQ, "");
	FTPPacket sendPkt = lastPkt;
	string sendStr = "";
	fp = fopen(fName, "w");

	sendStr = sendPkt.toString();
	if(sendto(sockfd, sendStr.c_str(), sendStr.length(), 0, (const sockaddr *)&serveraddr, (socklen_t)serverlen) < 0)
		error("ERROR in sendto 133");
	else
		PacketFactory::print(sendPkt);

	while(1)
	{
		if(simulateLoss())
		{
			//TODO: enable this part
			//sleep(RTO);// trigger timeout at server side, simulate packet lost
		}

		if(recvfrom(sockfd, buf, strlen(buf), 0, (sockaddr*)&serveraddr, (socklen_t *)&serverlen) < 0)
			error("ERROR in recvfrom");
		else // update received message, compose response.
		{
			sendPkt = parseRecv(string(buf),lastPkt);
		}

		if(sendPkt.payload == "ignore")
			continue;
		else if(sendPkt.payload == "done")
			break;

		sendStr = sendPkt.toString();
		if(sendto(sockfd, sendStr.c_str(), sendStr.length(), 0, (const sockaddr *)&serveraddr, (socklen_t)serverlen) < 0)
			error("ERROR in sendto");
		else
			printf("client.cpp 158: sent: %s\n", sendStr.c_str());
	}
	
	fclose(fp);
	//TODO close the connection
	//implement TCP-like closing method
	
    return 0;
}
