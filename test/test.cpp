#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <stdio.h>
#include "unistd.h"

int main ()
{
    FILE *fp;
    char str[] = "This is tutorialspoint.com";
    
    fp = fopen( "file.txt" , "rb" );
    char *s = new char[100];
	for(int i=0;i<3;i++)
	{
		fread(s, 1, 9, fp);
		printf("%s",s);
		puts("*===\n");
	}
    delete[] s;
    fclose(fp);
    
    return(0);
}
