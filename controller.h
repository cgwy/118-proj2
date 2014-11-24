#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "packet.h"
#include <unordered_map>

bool congestion_control_on;

//congestion control default parameters
int DEFAULT_CWND = 8;
int DEFAULT_SSTHRESH = 5;

int sendWinMax = 100;

//TODO: implement timer function using callback
struct SendController
{
protected:
	FILE *fp;
	size_t fSize;
	size_t readSize;
	bool readOver;

	vector<string> sendWindow;// implement a circular window
	int winLeft, winRight;// (inclusive)border of the window, the window stores all the sent and not ACKed packets
	size_t cwnd;
	size_t ssthresh;

	string readBlock()
	{
		size_t remain = fSize - readSize;
		int newReadSize = remain > MAX_PAYLOAD? MAX_PAYLOAD: remain;
		char buf[MAX_PAYLOAD+1]; 
		
		fread(buf, 1, remain, fp);
		readSize += newReadSize;
		if(fSize <= readSize)
			readOver = true;

		return string(buf);
	}
public:
	int lastAck;
	vector<string> getWind()
	{
		vector<string> res;
		if(winLeft == -1)
			return res;
		else
		{
			int i = winLeft;
			do
			{
				res.push_back(sendWindow[i]);
				i = (++i) % sendWinMax;
			}while(i!=winRight);
			return res;
		}
	}

	//clock_t timer;//start time of computing timeout
	SendController(const string fName)
	{  
		this->fp = fopen(fName.c_str(), "rb");
		fseek(fp, 0, SEEK_END);
		this->fSize = ftell(fp);
		this->readSize = 0;
		this->readOver = false;

		if(congestion_control_on)
			this->cwnd = 1;
		else
			this->cwnd = DEFAULT_CWND;
		this->ssthresh = DEFAULT_SSTHRESH;

		this->lastAck = -1;
		this->winLeft = this->winRight = -1;
		
		//TODO: seperate thread for tiemer 
		//this->timer = time;
	}  
	SendController(){ 
		this->fp = NULL;
	}
	
	~SendController(){ 
		fclose(fp);
	}

	void windAdd(string str)
	{
		if(winRight == sendWinMax - 1)
		{
			winRight = 0;
			sendWindow[0] = str;
		}
		else
		{
			winRight++;
			if(sendWindow.size() <= winRight)
				sendWindow.push_back(str);
			else
				sendWindow[winRight] = str;
		}	
	}
	
	void goBack(int n)
	{}
	
	vector<string> handleNewAck(int newAck)
	{
		vector<string> res;
		if(readOver)
			return res;
		
		if(!congestion_control_on)
		{
			if(winLeft == -1)
			{
				winLeft = 0;
				for(int i=0;i<cwnd;i++)
				{
					string str = readBlock();
					windAdd(str);
					res.push_back(str);
				}
			}
			else//window size won't change
			{
				winLeft++;
				string str = readBlock();
				windAdd(str);
				res.push_back(str);
			}
		}
		else
		{}

		return res;
	}
	vector<string> handleDupAck()
	{
		vector<string> res;
		if(!congestion_control_on)
			res = getWind();
		else
		{ 
			//TODO dup Ack count
		}
		return res;
	}
	vector<string> handleTimeout();
}; 

//input: event -- new request / new ack / repeat ack
//TODO timeout is triggered by the timers inside cootroller.
//output: response to be sent to client.
//
//implements congestion control
class Controller
{
	unordered_map<string, SendController> ctrls;

	public:
	Controller(){}
	
	vector<string> handleNewPkt(string host, string pktString)
	{
		vector<string> res;
		//check if incoming packet is corrupted
		FTPPacket pktRecv = FTPPacket::toPacket(pktString);
		if(pktRecv.isCorrupted())//corrupted ACK; resend packets in current window
			return ctrls[host].getWind();

		unordered_map<string, SendController>::iterator iter = ctrls.find(host);
		if(iter == ctrls.end())
		{
			fprintf(stderr, "NEW_REQ from host %s\n", host.c_str());
			string fName = pktRecv.payload;
			ctrls.insert( make_pair( host, SendController(fName)));
		}

		//TODO handle New ACK, reset timer for this host
		if(pktRecv.pktType == RECV_ACK)
		{
			if(pktRecv.ack == ctrls[host].lastAck)
			{
				fprintf(stderr, "DUP ACK from host %s\n", host.c_str());
				vector<string> respVec = ctrls[host].handleDupAck();
				if(respVec.size() > 0)// 3rd dup ACK
					return respVec;
				else{
					//do nothing for first 2dup ACK
				}
			}
			else
			{
				fprintf(stderr, "NEW ACK from host %s\n", host.c_str());
				return ctrls[host].handleNewAck(pktRecv.ack);
			}
		}
		else if(pktRecv.pktType == RECV_NAK)
		{
			fprintf(stderr, "ACK from host %s\n", host.c_str());
			return ctrls[host].getWind();
		}	
		else
		{
			fprintf(stderr, "wrong packet type");
			exit(-1);
		}
		return res;
	} 

	//check for timeout
	void handleTimer(clock_t time)
	{
		//TODO check time out
	}
};

#endif
