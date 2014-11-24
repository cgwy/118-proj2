#include <iostream>
#include "../common.h"

using namespace std;

int main()
{
	string s;
	for(int i=0;i<13*MAX_PAYLOAD/3;i++)
  	{
		char ch = i%26+'a';
		s = s+ch;
	}
 	FTPPacket pkt(50,100,s);
	vector<FTPPacket> pktSlices = pkt.slice();
	for(int i=0;i<pktSlices.size();i++)
		cout<<pktSlices[i].payload<<endl<<endl;
	
	return 0;
}
